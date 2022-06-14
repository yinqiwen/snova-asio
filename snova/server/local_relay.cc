/*
 *Copyright (c) 2022, qiyingwang <qiyingwang@tencent.com>
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
#include "snova/server/local_relay.h"

#include "absl/cleanup/cleanup.h"
#include "asio/experimental/as_tuple.hpp"
#include "asio/experimental/awaitable_operators.hpp"
#include "snova/io/transfer.h"
#include "snova/mux/mux_client.h"
#include "snova/util/flags.h"
#include "snova/util/net_helper.h"
#include "snova/util/time_wheel.h"

using namespace asio::experimental::awaitable_operators;
namespace snova {

template <typename T>
static asio::awaitable<void> do_client_relay(T& local_stream, const Bytes& readed_data,
                                             const std::string& remote_host, uint16_t remote_port,
                                             bool is_tcp) {
  bool direct_relay = false;
  // TODO
  if (direct_relay) {
    auto remote_socket = co_await get_connected_socket(remote_host, remote_port, is_tcp);
    if (remote_socket) {
      if (readed_data.size() > 0) {
        co_await ::asio::async_write(*remote_socket,
                                     ::asio::buffer(readed_data.data(), readed_data.size()),
                                     ::asio::experimental::as_tuple(::asio::use_awaitable));
      }
      try {
        co_await(transfer(local_stream, *remote_socket) && transfer(*remote_socket, local_stream));
      } catch (std::exception& ex) {
        SNOVA_ERROR("ex:{}", ex.what());
      }
    }
    co_return;
  }
  auto ex = co_await asio::this_coro::executor;
  EventWriterFactory factory = MuxClient::GetEventWriterFactory();
  uint64_t client_id = MuxClient::GetInstance()->GetClientId();
  uint32_t stream_id = MuxStream::NextID(true);
  MuxStreamPtr remote_stream = MuxStream::New(std::move(factory), ex, client_id, stream_id);
  absl::Cleanup auto_remove_remove_stream = [client_id, stream_id] {
    MuxStream::Remove(client_id, stream_id);
  };
  auto ec = co_await remote_stream->Open(remote_host, remote_port, is_tcp);
  if (readed_data.size() > 0) {
    IOBufPtr tmp_buf = get_iobuf(readed_data.size());
    memcpy(tmp_buf->data(), readed_data.data(), readed_data.size());
    co_await remote_stream->Write(std::move(tmp_buf), readed_data.size());
  }
  TransferRoutineFunc transfer_routine;
  uint32_t latest_io_time = time(nullptr);
  TimerTaskID transfer_timeout_task_id;
  if (g_stream_io_timeout_secs > 0) {
    transfer_routine = [&]() { latest_io_time = time(nullptr); };
    TimerTask timer_task;
    timer_task.timeout_callback = [&]() -> asio::awaitable<void> {
      SNOVA_ERROR("[{}]Close stream since it's not active since {}s ago.", stream_id,
                  time(nullptr) - latest_io_time);
      co_await remote_stream->Close(false);
      co_return;
    };
    timer_task.get_active_time = [&]() -> uint32_t { return latest_io_time; };
    timer_task.id_update_cb = [&](TimerTaskID& id) { transfer_timeout_task_id = id; };
    timer_task.timeout_secs = g_stream_io_timeout_secs;
    transfer_timeout_task_id = TimeWheel::GetInstance()->Register(std::move(timer_task));
  }

  try {
    co_await(transfer(local_stream, remote_stream, transfer_routine) &&
             transfer(remote_stream, local_stream, transfer_routine));
  } catch (std::exception& ex) {
    SNOVA_ERROR("ex:{}", ex.what());
  }
  if (g_stream_io_timeout_secs > 0) {
    TimeWheel::GetInstance()->Cancel(transfer_timeout_task_id);
  }

  co_await remote_stream->Close(false);
}

asio::awaitable<void> client_relay(::asio::ip::tcp::socket&& sock, const Bytes& readed_data,
                                   const std::string& remote_host, uint16_t remote_port,
                                   bool is_tcp) {
  ::asio::ip::tcp::socket socket(std::move(sock));  //  make rvalue sock not release after co_await
  co_await do_client_relay(socket, readed_data, remote_host, remote_port, is_tcp);
}

asio::awaitable<void> client_relay(StreamPtr local_stream, const Bytes& readed_data,
                                   const std::string& remote_host, uint16_t remote_port,
                                   bool is_tcp) {
  co_await do_client_relay(local_stream, readed_data, remote_host, remote_port, is_tcp);
  co_await local_stream->Close(false);
}

}  // namespace snova
