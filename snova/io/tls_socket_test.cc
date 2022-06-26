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
#include "snova/io/tls_socket.h"
#include <gtest/gtest.h>
#include <utility>
#include "snova/log/log_macros.h"
#include "snova/util/net_helper.h"
using namespace snova;  // NOLINT

TEST(TlsSocket, Simple) {
  ::asio::io_context ctx;
  TlsSocket socket(ctx.get_executor());

  ::asio::co_spawn(
      ctx,
      [socket = std::move(socket)]() mutable -> asio::awaitable<void> {
        co_await socket.AsyncConnect("www.baidu.com", 443);
        std::string req = "GET / HTTP/1.0\r\n\r\n";
        co_await socket.AsyncWrite(::asio::buffer(req.data(), req.size()));
        char buffer[4096];
        auto [n, ec] = co_await socket.AsyncRead(::asio::buffer(buffer, 4096));
        if (n > 0) {
          SNOVA_INFO("Recv:{}", std::string_view(buffer, n));
        }
      },
      ::asio::detached);
  ctx.run();
}

TEST(TlsSocket, Simple2) {
  ::asio::io_context ctx;

  ::asio::co_spawn(
      ctx,
      []() mutable -> asio::awaitable<void> {
        auto ex = co_await asio::this_coro::executor;
        ::asio::ip::tcp::socket sock(ex);
        TlsSocket socket(std::move(sock));
        co_await socket.AsyncConnect("www.baidu.com", 443);
        std::string req = "GET / HTTP/1.0\r\n\r\n";
        co_await socket.AsyncWrite(::asio::buffer(req.data(), req.size()));
        char buffer[4096];
        auto [n, ec] = co_await socket.AsyncRead(::asio::buffer(buffer, 4096));
        if (n > 0) {
          SNOVA_INFO("Recv:{}", std::string_view(buffer, n));
        }
      },
      ::asio::detached);
  ctx.run();
}