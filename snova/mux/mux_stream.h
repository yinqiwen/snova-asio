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

#pragma once
#include <memory>
#include <string>
#include <vector>
#include "asio.hpp"
#include "snova/io/io.h"
#include "snova/mux/mux_event.h"
namespace snova {
using StreamDataChannel = asio::experimental::channel<void(std::error_code, IOBufPtr&&, size_t)>;
using StreamDataChannelExecutor = typename StreamDataChannel::executor_type;
class MuxStream;
using MuxStreamPtr = std::shared_ptr<MuxStream>;
class MuxStream : public Stream {
 public:
  asio::awaitable<std::error_code> Open(const std::string& host, uint16_t port, bool is_tcp);
  asio::awaitable<std::error_code> Offer(IOBufPtr&& buf, size_t len);
  asio::awaitable<StreamReadResult> Read() override;
  asio::awaitable<std::error_code> Write(IOBufPtr&& buf, size_t len) override;
  asio::awaitable<std::error_code> Close(bool close_by_remote) override;
  ~MuxStream();

  static uint32_t NextID(bool is_client);
  static MuxStreamPtr New(EventWriterFactory&& factory, StreamDataChannelExecutor& ex,
                          uint32_t sid);
  static MuxStreamPtr Get(uint32_t sid);

 private:
  MuxStream(EventWriterFactory&& factory, StreamDataChannelExecutor& ex, uint32_t sid);
  template <typename T>
  asio::awaitable<bool> WriteEvent(std::unique_ptr<T>&& event) {
    std::unique_ptr<MuxEvent> write_ev = std::move(event);
    for (size_t i = 0; i < 3; i++) {  // try 3 times
      bool success = false;
      if (event_writer_) {
        success = co_await event_writer_(std::move(write_ev));
        if (success) {
          co_return true;
        }
      }
      event_writer_ = event_writer_factory_();
    }
    // close if failed 3 times
    co_await Close(false);
    co_return false;
  }
  EventWriterFactory event_writer_factory_;
  EventWriter event_writer_;
  StreamDataChannel data_channel_;
  uint32_t sid_;
  bool is_remote_closed_;
};

}  // namespace snova