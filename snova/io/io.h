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
#include <tuple>
#include <utility>
#include <vector>
#include "absl/types/span.h"
#include "asio.hpp"
#include "asio/experimental/awaitable_operators.hpp"
#include "snova/io/io.h"

namespace snova {
using IOResult = std::pair<size_t, std::error_code>;
using MutableBytes = absl::Span<uint8_t>;
using Bytes = absl::Span<const uint8_t>;
using IOBuf = std::vector<uint8_t>;
static constexpr uint16_t kMaxChunkSize = 8192;
static constexpr uint16_t kReservedBufferSize = 256;
struct IOBufDeleter {
  void operator()(IOBuf* v) const;
};
using IOBufSharedPtr = std::shared_ptr<IOBuf>;
using IOBufPtr = std::unique_ptr<IOBuf, IOBufDeleter>;

IOBufPtr get_iobuf(size_t n);
IOBufSharedPtr get_shared_iobuf(size_t n);

struct IOConnection {
  virtual asio::any_io_executor GetExecutor() = 0;
  virtual asio::awaitable<IOResult> AsyncWrite(const asio::const_buffer& buffers) = 0;
  virtual asio::awaitable<IOResult> AsyncWrite(
      const std::vector<::asio::const_buffer>& buffers) = 0;

  virtual asio::awaitable<IOResult> AsyncRead(const asio::mutable_buffer& buffers) = 0;
  virtual void Close() = 0;
  virtual ~IOConnection() = default;
};
using IOConnectionPtr = std::unique_ptr<IOConnection>;

using StreamReadResult = std::tuple<IOBufPtr, size_t, std::error_code>;
struct Stream {
  virtual uint32_t GetID() const = 0;
  virtual asio::awaitable<StreamReadResult> Read() = 0;
  virtual asio::awaitable<std::error_code> Write(IOBufPtr&& buf, size_t len) = 0;
  virtual asio::awaitable<std::error_code> Close(bool close_by_remote) = 0;
  virtual ~Stream() = default;
};
using StreamPtr = std::shared_ptr<Stream>;
using SocketRef = ::asio::ip::tcp::socket&;
using IOBufRef = IOBuf&;

void register_io_stat();

}  // namespace snova
