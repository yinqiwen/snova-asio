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
#include "asio/experimental/awaitable_operators.hpp"
#include "snova/io/transfer.h"
#include "snova/mux/mux_server.h"
#include "snova/mux/mux_stream.h"
#include "spdlog/fmt/bundled/ostream.h"

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
  MuxStreamPtr stream = MuxStream::New(std::move(factory), ex, open_request->head.sid);
  asio::ip::tcp::resolver r(ex);
  std::string port_str = std::to_string(open_request->remote_port);
  // SNOVA_INFO("1.5[{}]server_relay {}", open_request->head.sid, client_id);
  auto [ec, results] = co_await r.async_resolve(
      open_request->remote_host, port_str, ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (ec || results.size() == 0) {
    SNOVA_ERROR("No endpoint found for {}:{} with error:{}", open_request->remote_host, port_str,
                ec);
    co_await stream->Close(false);
    co_return;
  }
  // SNOVA_INFO("2[{}]server_relay {}", mux_event->head.sid, client_id);

  ::asio::ip::tcp::socket socket(ex);
  ::asio::ip::tcp::endpoint select_endpoint = *(results.begin());
  SNOVA_INFO("[{}]Relay to {}/{}", open_request->head.sid, open_request->remote_host,
             select_endpoint);
  auto [connect_ec] = co_await socket.async_connect(
      select_endpoint, ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (connect_ec) {
    SNOVA_ERROR("Connect {} with error:{}", select_endpoint, connect_ec);
    co_await stream->Close(false);
    co_return;
  }
  try {
    co_await(transfer_socket(socket, stream) && transfer_stream(stream, socket));
  } catch (std::exception& ex) {
    SNOVA_ERROR("ex:{}", ex.what());
  }
  co_await stream->Close(false);
}
}  // namespace snova
