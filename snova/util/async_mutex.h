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
#include "asio.hpp"
#include "asio/experimental/channel.hpp"
namespace snova {
using AsyncChannelMutexExecutor =
    typename asio::experimental::channel<void(std::error_code, bool)>::executor_type;
class AsyncChannelMutex {
 public:
  AsyncChannelMutex(const AsyncChannelMutexExecutor& ex);
  asio::awaitable<std::error_code> Lock();
  asio::awaitable<std::error_code> Unlock();
  void Close();

 private:
  asio::experimental::channel<void(std::error_code, bool)> channel_;
  size_t wait_count_;
  bool locked_;
};

// class AsyncSpinMutex {
//  public:
//   AsyncSpinMutex(const typename ::asio::steady_timer::executor_type& ex);
//   asio::awaitable<std::error_code> Lock();
//   asio::awaitable<std::error_code> Unlock();
//   void Close();

//  private:
//   bool locked_;
// };
}  // namespace snova