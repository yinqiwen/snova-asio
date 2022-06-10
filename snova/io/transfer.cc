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
#include "snova/io/transfer.h"
#include <vector>
#include "asio/experimental/as_tuple.hpp"
#include "snova/log/log_macros.h"
#include "spdlog/fmt/bundled/ostream.h"
namespace snova {
using namespace asio::experimental::awaitable_operators;

asio::awaitable<void> transfer(StreamPtr from, StreamPtr to) {
  while (true) {
    auto [data, len, ec] = co_await from->Read();
    if (ec) {
      // co_await from->Close(false);
      SNOVA_ERROR("Read ERROR {} {}", len, ec);
      break;
    }
    auto wec = co_await to->Write(std::move(data), len);
    if (wec) {
      // co_await from->Close(false);
      SNOVA_ERROR("async_write ERROR {} ", wec);
      break;
    }
  }
  co_return;
}

asio::awaitable<void> transfer(StreamPtr from, ::asio::ip::tcp::socket& to) {
  while (true) {
    auto [data, len, ec] = co_await from->Read();
    if (ec) {
      // co_await from->Close(false);
      SNOVA_ERROR("Read ERROR {} {}", len, ec);
      break;
    }
    auto [wec, wn] =
        co_await ::asio::async_write(to, ::asio::buffer(data->data(), len),
                                     ::asio::experimental::as_tuple(::asio::use_awaitable));
    data.reset();
    if (wec) {
      // co_await from->Close(false);
      SNOVA_ERROR("async_write ERROR {} ", wec);
      break;
    }
  }
  co_return;
}
asio::awaitable<void> transfer(::asio::ip::tcp::socket& from, StreamPtr to) {
  while (true) {
    IOBufPtr buf = get_iobuf(kMaxChunkSize);
    auto [ec, n] =
        co_await from.async_read_some(::asio::buffer(buf->data(), buf->size()),
                                      ::asio::experimental::as_tuple(::asio::use_awaitable));
    if (ec) {
      break;
    }

    auto wec = co_await to->Write(std::move(buf), n);
    if (wec) {
      break;
    }
  }
  co_return;
}

}  // namespace snova
