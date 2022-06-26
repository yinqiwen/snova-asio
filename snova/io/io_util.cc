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
#include "snova/io/io_util.h"
#include "snova/io/tcp_socket.h"
#include "snova/log/log_macros.h"

namespace snova {
asio::awaitable<int> read_until(IOConnection& socket, IOBufRef buf, size_t& readed_len,
                                std::string_view until) {
  int rc = 0;
  while (true) {
    std::string_view readed_view((const char*)buf.data(), readed_len);
    auto pos = readed_view.find(until);
    if (pos != std::string_view::npos) {
      co_return pos;
    }
    size_t rest = buf.size() - readed_len;
    if (rest < until.size()) {
      buf.resize(buf.size() * 2);
    }
    auto [n, ec] =
        co_await socket.AsyncRead(::asio::buffer(buf.data() + readed_len, buf.size() - readed_len));
    if (n > 0) {
      readed_len += n;
    } else {
      SNOVA_ERROR("read socket error:{} with n:{}", ec, n);
      rc = -1;
      break;
    }
  }
  co_return rc;
}
asio::awaitable<int> read_until(SocketRef socket, IOBufRef buf, size_t& readed_len,
                                std::string_view until) {
  TcpSocket tcp(socket);
  co_return co_await read_until(tcp, buf, readed_len, until);
}
asio::awaitable<std::error_code> read_exact(IOConnection& socket,
                                            const asio::mutable_buffer& buffers) {
  size_t pos = 0;
  size_t rest = buffers.size();
  uint8_t* read_buffer = reinterpret_cast<uint8_t*>(buffers.data());
  while (rest > 0) {
    auto [n, ec] = co_await socket.AsyncRead(::asio::buffer(read_buffer + pos, rest));
    if (ec) {
      co_return ec;
    }
    rest -= n;
    pos += n;
  }
  co_return std::error_code{};
}

}  // namespace snova
