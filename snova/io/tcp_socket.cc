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
#include "snova/io/tcp_socket.h"
#include <utility>

#include "asio/experimental/as_tuple.hpp"

namespace snova {
TcpSocket::TcpSocket(::asio::ip::tcp::socket& sock) : socket_(sock) {}
TcpSocket::TcpSocket(::asio::ip::tcp::socket&& sock)
    : own_socket_{std::make_unique<::asio::ip::tcp::socket>(std::move(sock))},
      socket_(*own_socket_) {}
asio::any_io_executor TcpSocket::GetExecutor() { return socket_.get_executor(); }
asio::awaitable<IOResult> TcpSocket::AsyncWrite(const asio::const_buffer& buffers) {
  auto [ec, n] = co_await ::asio::async_write(
      socket_, buffers, ::asio::experimental::as_tuple(::asio::use_awaitable));
  co_return IOResult{n, ec};
}
asio::awaitable<IOResult> TcpSocket::AsyncRead(const asio::mutable_buffer& buffers) {
  auto [ec, n] = co_await socket_.async_read_some(
      buffers, ::asio::experimental::as_tuple(::asio::use_awaitable));
  co_return IOResult{n, ec};
}
void TcpSocket::Close() { socket_.close(); }
}  // namespace snova
