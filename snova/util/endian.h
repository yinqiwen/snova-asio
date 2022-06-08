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
#include "absl/base/internal/endian.h"
namespace snova {
template <typename T>
static T native_to_big(T v) {
  if constexpr (std::is_same_v<uint64_t, T>) {
    return absl::ghtonll(v);
  } else if constexpr (std::is_same_v<uint32_t, T>) {
    return absl::ghtonl(v);
  } else if constexpr (std::is_same_v<uint16_t, T>) {
    return absl::ghtons(v);
  } else {
    static_assert(sizeof(T) == std::size_t(-1), "Not support integer type.");
  }
}

template <typename T>
static T big_to_native(T v) {
  if constexpr (std::is_same_v<uint64_t, T>) {
    return absl::gntohll(v);
  } else if constexpr (std::is_same_v<uint32_t, T>) {
    return absl::gntohl(v);
  } else if constexpr (std::is_same_v<uint16_t, T>) {
    return absl::gntohs(v);
  } else {
    static_assert(sizeof(T) == std::size_t(-1), "Not support integer type.");
  }
}
}  // namespace snova