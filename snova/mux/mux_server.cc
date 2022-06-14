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
#include "snova/mux/mux_server.h"
#include <atomic>
namespace snova {
std::shared_ptr<MuxServer>& MuxServer::GetInstance() {
  static std::shared_ptr<MuxServer> g_instance = std::make_shared<MuxServer>();
  return g_instance;
}
EventWriterFactory MuxServer::GetEventWriterFactory(uint64_t client_id) {
  EventWriterFactory f = [client_id, this]() mutable -> EventWriter {
    auto found = mux_conns_.find(client_id);
    if (found == mux_conns_.end()) {
      return {};
    }
    auto& conns = found->second;
    uint32_t min_send_bytes = 0;
    int32_t select_idx = -1;
    for (size_t i = 0; i < conns.size(); i++) {
      if (!conns[i]) {
        continue;
      }
      if (-1 == select_idx || conns[i]->GetSendBytes() < min_send_bytes) {
        select_idx = i;
        min_send_bytes = conns[i]->GetSendBytes();
      }
    }
    if (select_idx >= 0) {
      return std::bind(&MuxConnection::Write, conns[select_idx], std::placeholders::_1);
    }
    return {};
  };
  return f;
}

uint32_t MuxServer::Add(uint64_t client_id, MuxConnectionPtr conn) {
  MuxConnArray& conns = mux_conns_[client_id];
  size_t idx = 0;
  for (auto& c : conns) {
    if (!c) {
      c = conn;
      return idx;
    }
    idx++;
  }
  conns.emplace_back(conn);
  return conns.size() - 1;
}
void MuxServer::Remove(uint64_t client_id, MuxConnection* conn) {
  auto found = mux_conns_.find(client_id);
  if (found == mux_conns_.end()) {
    return;
  }
  bool all_empty = true;
  bool match = false;
  for (auto& c : found->second) {
    if (c.get() == conn) {
      c = nullptr;
      match = true;
    }
    if (c) {
      all_empty = false;
      if (match) {
        break;
      }
    }
  }
  if (all_empty) {
    mux_conns_.erase(found);
  }
}
void MuxServer::Remove(uint64_t client_id, MuxConnectionPtr conn) { Remove(client_id, conn.get()); }
}  // namespace snova
