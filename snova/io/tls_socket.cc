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
#include "snova/io/tls_socket.h"
#include <string>
#include <utility>
#include "asio/experimental/as_tuple.hpp"

namespace snova {
TlsSocket::TlsSocket(::asio::ip::tcp::socket&& sock)
    : tls_ctx_(::asio::ssl::context::tls), tls_socket_(std::move(sock), tls_ctx_) {}
TlsSocket::TlsSocket(const ASIOTlsSocketExecutor& ex)
    : tls_ctx_(::asio::ssl::context::tls), tls_socket_(ex, tls_ctx_) {}
asio::any_io_executor TlsSocket::GetExecutor() { return tls_socket_.get_executor(); }
asio::awaitable<std::error_code> TlsSocket::ClientHandshake() {
  auto [handshake_ec] = co_await tls_socket_.async_handshake(
      ::asio::ssl::stream_base::client, ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (handshake_ec) {
    co_return handshake_ec;
  }
  co_return std::error_code{};
}
asio::awaitable<std::error_code> TlsSocket::AsyncConnect(const std::string& host, uint16_t port) {
  ::asio::ip::tcp::endpoint endpoint;
  auto ec = co_await resolve_endpoint(host, port, &endpoint);
  if (ec) {
    co_return ec;
  }
  auto [connect_ec] = co_await tls_socket_.next_layer().async_connect(
      endpoint, ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (connect_ec) {
    co_return connect_ec;
  }
  co_return co_await ClientHandshake();
}
asio::awaitable<IOResult> TlsSocket::AsyncWrite(const asio::const_buffer& buffers) {
  auto [ec, n] = co_await ::asio::async_write(
      tls_socket_, buffers, ::asio::experimental::as_tuple(::asio::use_awaitable));
  co_return IOResult{n, ec};
}
asio::awaitable<IOResult> TlsSocket::AsyncRead(const asio::mutable_buffer& buffers) {
  auto [ec, n] = co_await tls_socket_.async_read_some(
      buffers, ::asio::experimental::as_tuple(::asio::use_awaitable));
  co_return IOResult{n, ec};
}
void TlsSocket::Close() {
  tls_socket_.shutdown();
  tls_socket_.next_layer().close();
}
}  // namespace snova
