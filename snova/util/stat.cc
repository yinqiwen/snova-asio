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
#include "snova/util/stat.h"
#include <chrono>
#include <vector>
#include "absl/container/btree_map.h"
#include "absl/container/btree_set.h"
#include "snova/log/log_macros.h"

namespace snova {
using StatMap = absl::btree_map<std::string, std::string>;
using StatTable = absl::btree_map<std::string, StatMap>;

static StatTable g_stat_table;
static std::vector<CollectStatFunc> g_stat_funcs;

void register_stat_func(CollectStatFunc&& func) { g_stat_funcs.emplace_back(std::move(func)); }

static void print_stats() {
  g_stat_table.clear();
  for (auto& f : g_stat_funcs) {
    auto stat_vals = f();
    for (const auto& [sec, kvs] : stat_vals) {
      for (const auto& [k, v] : kvs) {
        g_stat_table[sec][k] = v;
      }
    }
  }
  if (g_stat_table.empty()) {
    return;
  }
  std::string buffer;
  buffer.append("======================Stats=====================\n");
  for (const auto& [section, map] : g_stat_table) {
    buffer.append("[").append(section).append("]:\n");
    for (const auto& [key, value] : map) {
      buffer.append("  ").append(key).append(": ").append(value).append("\n");
    }
    buffer.append("\n");
  }
  SNOVA_INFO("{}", buffer);
}
asio::awaitable<void> start_stat_timer(uint32_t period_secs) {
  auto ex = co_await asio::this_coro::executor;
  ::asio::steady_timer timer(ex);
  std::chrono::seconds period(period_secs);
  while (true) {
    timer.expires_after(period);
    co_await timer.async_wait(::asio::use_awaitable);
    print_stats();
  }
  co_return;
}

}  // namespace snova
