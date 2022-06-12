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
#include "snova/io/io.h"
#include <stack>
#include "snova/util/flags.h"
#include "snova/util/stat.h"

namespace snova {
static std::stack<IOBuf*> g_io_bufs;
// static constexpr uint32_t kIOBufPoolSize = 64;
static uint64_t g_iobuf_pool_bytes = 0;
static uint64_t g_active_iobuf_bytes = 0;
static uint32_t g_active_iobuf_num = 0;

void register_io_stat() {
  register_stat_func([]() -> StatValues {
    StatValues vals;
    auto& kv = vals["IOBuf"];
    kv["pool_size"] = std::to_string(g_io_bufs.size());
    kv["pool_bytes"] = std::to_string(g_iobuf_pool_bytes);
    kv["active_iobuf_num"] = std::to_string(g_active_iobuf_num);
    kv["active_iobuf_bytes"] = std::to_string(g_active_iobuf_bytes);
    return vals;
  });
}

void IOBufDeleter::operator()(IOBuf* v) const {
  g_active_iobuf_num--;
  g_active_iobuf_bytes -= v->capacity();
  if (g_io_bufs.size() < g_iobuf_max_pool_size) {
    g_io_bufs.push(v);
    g_iobuf_pool_bytes += v->capacity();
    return;
  }
  delete v;
}
static IOBuf* get_raw_iobuf(size_t n) {
  IOBuf* p = nullptr;
  bool fetch_from_pool = false;
  if (g_io_bufs.empty()) {
    p = new IOBuf;
  } else {
    p = g_io_bufs.top();
    g_io_bufs.pop();
    fetch_from_pool = true;
  }
  if (p->size() < n) {
    p->resize(n);
  }
  if (fetch_from_pool) {
    g_iobuf_pool_bytes -= p->capacity();
  }
  g_active_iobuf_num++;
  g_active_iobuf_bytes += p->capacity();
  return p;
}

IOBufPtr get_iobuf(size_t n) {
  IOBufDeleter deleter;
  IOBufPtr ptr(get_raw_iobuf(n), deleter);
  return ptr;
}
IOBufSharedPtr get_shared_iobuf(size_t n) {
  IOBufDeleter deleter;
  IOBufSharedPtr ptr(get_raw_iobuf(n), deleter);
  return ptr;
}

}  // namespace snova
