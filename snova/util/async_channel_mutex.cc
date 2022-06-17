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
#include "snova/util/async_channel_mutex.h"
#include "asio/experimental/as_tuple.hpp"

namespace snova {
AsyncChannelMutex::AsyncChannelMutex(const AsyncChannelMutexExecutor& ex)
    : channel_(ex, 1), wait_count_(0), locked_(false) {}
void AsyncChannelMutex::Close() {
  channel_.cancel();
  channel_.close();
}
asio::awaitable<std::error_code> AsyncChannelMutex::Lock() {
  if (!channel_.is_open()) {
    co_return std::error_code{};
  }
  while (locked_) {
    wait_count_++;
    auto [ec, v] =
        co_await channel_.async_receive(::asio::experimental::as_tuple(::asio::use_awaitable));
    wait_count_--;
    if (ec) {
      co_return ec;
    }
  }
  locked_ = true;
  co_return std::error_code{};
}
asio::awaitable<std::error_code> AsyncChannelMutex::Unlock() {
  if (!channel_.is_open()) {
    co_return std::error_code{};
  }
  locked_ = false;
  if (wait_count_ > 0) {
    auto [ec] = co_await channel_.async_send(std::error_code{}, true,
                                             ::asio::experimental::as_tuple(::asio::use_awaitable));
    co_return ec;
  }
  co_return std::error_code{};
}

// AsyncSpinMutex::AsyncSpinMutex(const typename ::asio::steady_timer::executor_type& ex)
//     : locked_(false) {}
// asio::awaitable<std::error_code> AsyncSpinMutex::Lock() {
//   std::chrono::milliseconds wait_period(1);
//   std::unique_ptr<::asio::steady_timer> timer;
//   while (locked_) {
//     if (!timer) {
//       auto ex = co_await asio::this_coro::executor;
//       timer = std::make_unique<::asio::steady_timer>(ex);
//     }
//     timer->expires_after(wait_period);
//     co_await timer->async_wait(::asio::use_awaitable);
//   }
//   locked_ = true;
//   co_return std::error_code{};
// }
// asio::awaitable<std::error_code> AsyncSpinMutex::Unlock() {
//   locked_ = false;
//   co_return std::error_code{};
// }
// void AsyncSpinMutex::Close() {}

}  // namespace snova
