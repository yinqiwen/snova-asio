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
#include "snova/io/ws_socket.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <utility>
#include "snova/io/tcp_socket.h"
#include "snova/log/log_macros.h"
#include "snova/util/address.h"
#include "snova/util/net_helper.h"
using namespace snova;  // NOLINT

TEST(WebSocket, Simple) {
  ::asio::io_context ctx;
  ::asio::ip::tcp::socket socket(ctx);
  PaserAddressResult parse_result = NetAddress::Parse("127.0.0.1:8080");
  if (parse_result.second) {
    SNOVA_INFO("Parse failed:{}", parse_result.second);
    return;
  }
  auto remote_addr = std::move(parse_result.first);
  ::asio::co_spawn(
      ctx,
      [socket = std::move(socket),
       remote_addr = std::move(remote_addr)]() mutable -> asio::awaitable<void> {
        ::asio::ip::tcp::endpoint remote_endpoint;
        co_await remote_addr->GetEndpoint(&remote_endpoint);
        SNOVA_INFO("endpoint:{}", remote_endpoint);
        co_await socket.async_connect(remote_endpoint, ::asio::use_awaitable);

        IOConnectionPtr io = std::make_unique<TcpSocket>(std::move(socket));
        WebSocket ws(std::move(io));
        auto ec = co_await ws.AsyncConnect("localhost:8080");
        size_t i = 0;
        while (true) {
          std::string s = "hello,world!";
          s.append(std::to_string(i));
          // SNOVA_INFO("Try send:{}", s.size());
          co_await ws.AsyncWrite(::asio::buffer(s.data(), s.size()));
          s.clear();
          char buffer[1024];
          auto [n, ec] = co_await ws.AsyncRead(::asio::buffer(buffer, 1024));
          if (n > 0) {
            // SNOVA_INFO("[{}]Recv:{}", n, std::string_view(buffer, n));
          }
          i++;
        }
      },
      ::asio::detached);
  ctx.run();
}
