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
#include "snova/server/remote_relay.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "asio/experimental/awaitable_operators.hpp"
#include "snova/io/transfer.h"
#include "snova/mux/mux_server.h"
#include "snova/mux/mux_stream.h"
#include "snova/server/local_relay.h"
#include "snova/util/net_helper.h"

ABSL_DECLARE_FLAG(bool, middle);

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
  MuxStreamPtr local_stream = MuxStream::New(std::move(factory), ex, open_request->head.sid);

  if (absl::GetFlag(FLAGS_middle)) {
    co_await client_relay(local_stream, Bytes{}, open_request->remote_host,
                          open_request->remote_port, open_request->is_tcp);
  } else {
    auto remote_socket = co_await get_connected_socket(
        open_request->remote_host, open_request->remote_port, open_request->is_tcp);

    if (!remote_socket) {
      co_await local_stream->Close(false);
      co_return;
    }
    try {
      co_await(transfer(*remote_socket, local_stream) && transfer(local_stream, *remote_socket));
    } catch (std::exception& ex) {
      SNOVA_ERROR("ex:{}", ex.what());
    }
    co_await local_stream->Close(false);
  }
}
}  // namespace snova
