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

#pragma once
#include <memory>
#include <string>
#include <vector>
#include "asio.hpp"
#include "asio/experimental/as_tuple.hpp"
#include "asio/ssl.hpp"
#include "snova/io/io.h"
#include "snova/util/net_helper.h"
namespace snova {
using ASIOTlsSocket = ::asio::ssl::stream<::asio::ip::tcp::socket>;
using ASIOTlsSocketExecutor = typename ASIOTlsSocket::executor_type;
class TlsSocket : public IOConnection {
 public:
  explicit TlsSocket(::asio::ip::tcp::socket&& sock);
  explicit TlsSocket(const ASIOTlsSocketExecutor& ex);
  void SetHost(const std::string& v) { tls_host_ = v; }
  asio::any_io_executor GetExecutor() override;
  asio::awaitable<std::error_code> ClientHandshake();
  asio::awaitable<std::error_code> AsyncConnect(const ::asio::ip::tcp::endpoint& endpoint);
  asio::awaitable<std::error_code> AsyncConnect(const std::string& host, uint16_t port);
  asio::awaitable<IOResult> AsyncWrite(const asio::const_buffer& buffers) override {
    co_return co_await DoAsyncWrite(buffers);
  }
  asio::awaitable<IOResult> AsyncWrite(const std::vector<::asio::const_buffer>& buffers) override {
    co_return co_await DoAsyncWrite(buffers);
  }
  asio::awaitable<IOResult> AsyncRead(const asio::mutable_buffer& buffers) override;
  void Close() override;

 private:
  template <typename T>
  asio::awaitable<IOResult> DoAsyncWrite(const T& buffers) {
    auto [ec, n] = co_await ::asio::async_write(
        tls_socket_, buffers, ::asio::experimental::as_tuple(::asio::use_awaitable));
    co_return IOResult{n, ec};
  }
  ::asio::ssl::context tls_ctx_;
  ASIOTlsSocket tls_socket_;
  std::string tls_host_;
};
}  // namespace snova
