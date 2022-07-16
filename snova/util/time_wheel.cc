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

#include "snova/util/time_wheel.h"
#include <time.h>
#include <chrono>

#include "asio/experimental/as_tuple.hpp"
#include "snova/util/flags.h"

namespace snova {
using std::chrono::milliseconds;

struct TimerTask {
  TimeoutFunc timeout_callback;
  GetActiveTimeFunc get_active_time;
  uint64_t timeout_msecs = 0;
  uint64_t create_time = 0;
  bool canceled = false;
};

std::shared_ptr<TimeWheel>& TimeWheel::GetInstance() {
  static std::shared_ptr<TimeWheel> g_instance = std::make_shared<TimeWheel>(128);
  return g_instance;
}

TimeWheel::TimeWheel(uint32_t slot_size) { time_wheel_.resize(slot_size); }

CancelFunc TimeWheel::Add(TimeoutFunc&& func, GetActiveTimeFunc&& active, uint64_t timeout_msecs) {
  TimerTaskPtr task = std::make_shared<TimerTask>();
  task->timeout_callback = std::move(func);
  task->get_active_time = std::move(active);
  task->timeout_msecs = timeout_msecs;
  return DoRegister(task);
}
CancelFunc TimeWheel::Add(TimeoutFunc&& func, uint64_t timeout_msecs) {
  TimerTaskPtr task = std::make_shared<TimerTask>();
  task->timeout_callback = std::move(func);
  task->timeout_msecs = timeout_msecs;
  return DoRegister(task);
}

CancelFunc TimeWheel::DoRegister(const TimerTaskPtr& rtask) {
  TimerTaskPtr task = rtask;
  if (task->timeout_msecs == 0) {
    task->timeout_msecs = 500;
  }
  auto now =
      std::chrono::duration_cast<milliseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
  // uint32_t now = time(nullptr);
  task->create_time = now;
  size_t idx = GetIdxByMillsecs(now + task->timeout_msecs);
  auto cancel_func = [task]() {
    task->canceled = true;
    task->timeout_callback = {};
    task->get_active_time = {};
  };
  time_wheel_[idx].emplace_back(task);
  return cancel_func;
}

size_t TimeWheel::GetIdxByMillsecs(uint64_t ms) { return (ms / 10) % time_wheel_.size(); }

asio::awaitable<void> TimeWheel::Run() {
  auto ex = co_await asio::this_coro::executor;
  ::asio::steady_timer timer(ex);
  std::chrono::milliseconds period(99);  // run every 99ms
  while (true) {
    timer.expires_after(period);
    co_await timer.async_wait(::asio::experimental::as_tuple(::asio::use_awaitable));
    auto now = std::chrono::duration_cast<milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();
    size_t idx = GetIdxByMillsecs(now);
    if (time_wheel_[idx].empty()) {
      continue;
    }
    TimerTaskQueue routine_queue = std::move(time_wheel_[idx]);
    for (auto& task : routine_queue) {
      if (task->canceled) {
        continue;
      }
      uint64_t active_time = task->create_time;
      if (task->get_active_time) {
        active_time = task->get_active_time();
      }
      if (now - active_time > task->timeout_msecs) {
        co_await task->timeout_callback();
      } else {
        uint32_t next_idx = GetIdxByMillsecs(active_time + task->timeout_msecs + 100);
        time_wheel_[next_idx].emplace_back(std::move(task));
      }
    }
    routine_queue.clear();
  }
}
}  // namespace snova
