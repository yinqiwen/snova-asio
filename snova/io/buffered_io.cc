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
#include "snova/io/buffered_io.h"
#include <utility>

#include "asio/experimental/as_tuple.hpp"

namespace snova {
BufferedIO::BufferedIO(IOConnectionPtr&& io) : io_(std::move(io)), offset_(0), length_(0) {
  recv_buffer_ = get_iobuf(kMaxChunkSize);
}

asio::any_io_executor BufferedIO::GetExecutor() { return io_->GetExecutor(); }
asio::awaitable<IOResult> BufferedIO::AsyncWrite(const asio::const_buffer& buffers) {
  co_return co_await io_->AsyncWrite(buffers);
}
asio::awaitable<IOResult> BufferedIO::AsyncWrite(const std::vector<::asio::const_buffer>& buffers) {
  co_return co_await io_->AsyncWrite(buffers);
}
asio::awaitable<IOResult> BufferedIO::AsyncRead(const asio::mutable_buffer& buffers) {
  if (offset_ >= length_) {
    offset_ = 0;
    length_ = 0;
    auto [n, ec] = co_await io_->AsyncRead(::asio::buffer(recv_buffer_->data(), kMaxChunkSize));
    if (ec) {
      co_return IOResult{0, ec};
    }
    length_ = n;
  }
  if (offset_ < length_) {
    size_t read_len = (length_ - offset_);
    if (buffers.size() < read_len) {
      read_len = buffers.size();
    }
    memcpy(buffers.data(), recv_buffer_->data() + offset_, read_len);
    offset_ += read_len;
    co_return IOResult{read_len, std::error_code{}};
  }
  co_return IOResult{0, std::error_code{}};
}
void BufferedIO::Close() { io_->Close(); }
}  // namespace snova
