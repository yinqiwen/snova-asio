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
#include "snova/io/transfer.h"
#include <utility>
#include <vector>
#include "asio/experimental/as_tuple.hpp"

namespace snova {
using namespace asio::experimental::awaitable_operators;  // NOLINT

asio::awaitable<void> transfer(StreamPtr from, StreamPtr to, const TransferRoutineFunc& routine) {
  while (true) {
    auto [data, len, ec] = co_await from->Read();
    if (ec) {
      // co_await from->Close(false);
      // SNOVA_ERROR("Read ERROR {} {}", len, ec);
      break;
    }
    if (routine) {
      routine();
    }
    auto wec = co_await to->Write(std::move(data), len);
    if (wec) {
      // co_await from->Close(false);
      // SNOVA_ERROR("async_write ERROR {} ", wec);
      break;
    }
    if (routine) {
      routine();
    }
  }
  co_await to->Close(false);
  co_return;
}

asio::awaitable<void> transfer(StreamPtr from, SocketRef to, const TransferRoutineFunc& routine) {
  while (true) {
    auto [data, len, ec] = co_await from->Read();
    if (ec) {
      // co_await from->Close(false);
      // SNOVA_ERROR("[{}]Read ERROR {}", from->GetID(), ec);
      break;
    }
    if (routine) {
      routine();
    }
    auto [wec, wn] =
        co_await ::asio::async_write(to, ::asio::buffer(data->data(), len),
                                     ::asio::experimental::as_tuple(::asio::use_awaitable));
    data.reset();
    if (wec) {
      // co_await from->Close(false);
      // SNOVA_ERROR("async_write ERROR {} ", wec);
      break;
    }
    if (routine) {
      routine();
    }
  }
  co_await from->Close(false);
  to.close();
  co_return;
}
asio::awaitable<void> transfer(SocketRef from, StreamPtr to, const TransferRoutineFunc& routine) {
  while (true) {
    IOBufPtr buf = get_iobuf(kMaxChunkSize);
    auto [ec, n] =
        co_await from.async_read_some(::asio::buffer(buf->data(), kMaxChunkSize),
                                      ::asio::experimental::as_tuple(::asio::use_awaitable));
    if (ec) {
      break;
    }
    if (routine) {
      routine();
    }

    auto wec = co_await to->Write(std::move(buf), n);
    if (wec) {
      break;
    }
    if (routine) {
      routine();
    }
  }
  co_await to->Close(false);
  co_return;
}
asio::awaitable<void> transfer(SocketRef from, SocketRef to, const TransferRoutineFunc& routine) {
  IOBufPtr buf = get_iobuf(kMaxChunkSize);
  while (true) {
    auto [ec, n] =
        co_await from.async_read_some(::asio::buffer(buf->data(), kMaxChunkSize),
                                      ::asio::experimental::as_tuple(::asio::use_awaitable));
    if (ec) {
      break;
    }
    if (routine) {
      routine();
    }
    auto [wec, wn] = co_await ::asio::async_write(
        to, ::asio::buffer(buf->data(), n), ::asio::experimental::as_tuple(::asio::use_awaitable));
    if (wec) {
      break;
    }
    if (routine) {
      routine();
    }
  }
  co_return;
}

}  // namespace snova
