/*
 *Copyright (c) 2022, yinqiwen <yinqiwen@gmail.com>
 *All rights reserved.
 *
 *Redistribution and use in source and binary forms, with or without
 *modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of rimos nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "snova/server/remote_relay.h"
#include "absl/cleanup/cleanup.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "asio/experimental/awaitable_operators.hpp"
#include "snova/io/transfer.h"
#include "snova/mux/mux_server.h"
#include "snova/mux/mux_stream.h"
#include "snova/server/local_relay.h"
#include "snova/util/flags.h"
#include "snova/util/net_helper.h"
#include "snova/util/time_wheel.h"

namespace snova {
using namespace asio::experimental::awaitable_operators;
asio::awaitable<void> server_relay(uint64_t client_id, std::unique_ptr<MuxEvent>&& event) {
  StreamOpenRequest* open_request = dynamic_cast<StreamOpenRequest*>(event.get());
  std::unique_ptr<MuxEvent> mux_event =
      std::move(event);  // this make rvalue event not release after co_await
  if (nullptr == open_request) {
    SNOVA_ERROR("null request for EVENT_STREAM_OPEN");
    co_return;
  }
  auto ex = co_await asio::this_coro::executor;
  EventWriterFactory factory = MuxServer::GetInstance()->GetEventWriterFactory(client_id);
  uint32_t local_stream_id = open_request->head.sid;
  MuxStreamPtr local_stream = MuxStream::New(std::move(factory), ex, client_id, local_stream_id);
  absl::Cleanup auto_remove_local_stream = [client_id, local_stream_id] {
    MuxStream::Remove(client_id, local_stream_id);
  };

  if (g_is_middle_node) {
    co_await client_relay(local_stream, Bytes{}, open_request->remote_host,
                          open_request->remote_port, open_request->is_tcp);
  } else {
    auto remote_socket = co_await get_connected_socket(
        open_request->remote_host, open_request->remote_port, open_request->is_tcp);
    if (!remote_socket) {
      co_await local_stream->Close(false);
      co_return;
    }
    TransferRoutineFunc transfer_routine;
    uint32_t latest_io_time = time(nullptr);
    TimerTaskID transfer_timeout_task_id;
    if (g_stream_io_timeout_secs > 0) {
      transfer_routine = [&]() { latest_io_time = time(nullptr); };
      TimerTask timer_task;
      timer_task.timeout_callback = [&]() -> asio::awaitable<void> {
        SNOVA_ERROR("[{}]Close stream since it's not active since {}s ago.", local_stream_id,
                    time(nullptr) - latest_io_time);
        co_await local_stream->Close(false);
        co_return;
      };
      timer_task.get_active_time = [&]() -> uint32_t { return latest_io_time; };
      timer_task.id_update_cb = [&](TimerTaskID& id) { transfer_timeout_task_id = id; };
      timer_task.timeout_secs = g_stream_io_timeout_secs;
      transfer_timeout_task_id = TimeWheel::GetInstance()->Register(std::move(timer_task));
    }

    try {
      co_await(transfer(*remote_socket, local_stream, transfer_routine) &&
               transfer(local_stream, *remote_socket, transfer_routine));
    } catch (std::exception& ex) {
      SNOVA_ERROR("ex:{}", ex.what());
    }
    if (g_stream_io_timeout_secs > 0) {
      TimeWheel::GetInstance()->Cancel(transfer_timeout_task_id);
    }
    co_await local_stream->Close(false);
  }
}
}  // namespace snova
