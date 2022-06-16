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
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include "asio.hpp"

namespace snova {
using TimeoutFunc = std::function<asio::awaitable<void>()>;
using GetActiveTimeFunc = std::function<uint32_t()>;
using CancelFunc = std::function<void()>;

struct TimerTask;
using TimerTaskPtr = std::shared_ptr<TimerTask>;

class TimeWheel {
 public:
  static std::shared_ptr<TimeWheel>& GetInstance();
  TimeWheel(uint32_t max_timeout_secs = 600);
  CancelFunc Add(TimeoutFunc&& func, GetActiveTimeFunc&& active, uint32_t timeout_secs);
  CancelFunc Add(TimeoutFunc&& func, uint32_t timeout_secs);

  asio::awaitable<void> Run();

 private:
  CancelFunc DoRegister(TimerTaskPtr& task);
  using TimerTaskQueue = std::vector<TimerTaskPtr>;
  using TimeWheelQueue = std::vector<TimerTaskQueue>;
  TimeWheelQueue time_wheel_;
  uint32_t max_timeout_secs_;
};
}  // namespace snova