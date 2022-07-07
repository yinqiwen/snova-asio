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
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "asio.hpp"
#include "snova/mux/mux_connection.h"
#include "snova/util/stat.h"
namespace snova {

struct MuxSession : public std::enable_shared_from_this<MuxSession> {
  using MuxConnArray = std::vector<MuxConnectionPtr>;
  MuxConnArray conns;
  std::vector<uint64_t> tunnel_servers;
  EventWriterFactory GetEventWriterFactory();
  void ReportStatInfo(StatKeyValue& kv);
  ~MuxSession();
};
using MuxSessionPtr = std::shared_ptr<MuxSession>;
struct UserMuxConn {
  using MuxConnTable = absl::flat_hash_map<uint64_t, MuxSessionPtr>;
  std::string user;
  MuxConnTable sessions[2];  // 0:entry 1:exit
};

using UserMuxConnPtr = std::shared_ptr<UserMuxConn>;

class MuxConnManager {
 public:
  static std::shared_ptr<MuxConnManager>& GetInstance();

  void RegisterStat();

  MuxSessionPtr GetSession(std::string_view user, uint64_t client_id, MuxConnectionType type);
  MuxSessionPtr Add(std::string_view user, uint64_t client_id, MuxConnectionPtr conn,
                    uint32_t* idx);
  void Remove(std::string_view user, uint64_t client_id, MuxConnectionPtr conn);
  void Remove(std::string_view user, uint64_t client_id, MuxConnection* conn);

  EventWriterFactory GetEventWriterFactory(std::string_view user, uint64_t client_id,
                                           MuxConnectionType type);
  EventWriterFactory GetRelayEventWriterFactory(std::string_view user, uint64_t* client_id);

 private:
  void ReportStatInfo(StatValues& stats);
  UserMuxConnPtr GetUserMuxConn(std::string_view user);
  using UserMuxConnTable = absl::flat_hash_map<std::string_view, UserMuxConnPtr>;
  UserMuxConnTable mux_conns_;
};

}  // namespace snova
