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
#include "snova/mux/mux_conn_manager.h"
#include <utility>
#include "snova/server/tunnel_server.h"
#include "snova/util/flags.h"
namespace snova {
MuxSession::~MuxSession() {
  for (auto server_id : tunnel_servers) {
    close_tunnel_server(server_id);
  }
}
void MuxSession::ReportStatInfo(StatKeyValue& kv) {
  uint32_t now = time(nullptr);
  for (size_t i = 0; i < conns.size(); i++) {
    auto& conn = conns[i];
    if (!conn) {
      kv[fmt::format("[{}]", i)] = "NULL";
    } else {
      // kv[fmt::format("[{}]read_state", i)] = conn->GetReadState();
      auto inactive_secs = now - conn->GetLastActiveUnixSecs();
      kv[fmt::format("[{}]inactive_secs", i)] = std::to_string(inactive_secs);
      kv[fmt::format("[{}]recv_bytes", i)] = std::to_string(conn->GetRecvBytes());
      kv[fmt::format("[{}]send_bytes", i)] = std::to_string(conn->GetSendBytes());
      kv[fmt::format("[{}]latest_30s_recv_bytes", i)] =
          std::to_string(conn->GetLatestWindowRecvBytes());
      kv[fmt::format("[{}]latest_30s_send_bytes", i)] =
          std::to_string(conn->GetLatestWindowSendBytes());
      if (inactive_secs > g_connection_max_inactive_secs) {
        conn->Close();
      }
    }
  }
}
EventWriterFactory MuxSession::GetEventWriterFactory() {
  MuxSessionPtr session = shared_from_this();
  EventWriterFactory f = [session]() mutable -> EventWriter {
    int32_t select_idx = -1;
    for (size_t i = 0; i < session->conns.size(); i++) {
      if (!session->conns[i]) {
        continue;
      }
      if (-1 == select_idx) {
        select_idx = i;
      } else {
        if (session->conns[i]->ComparePriority(*(session->conns[select_idx])) > 0) {
          select_idx = i;
        }
      }
    }
    if (select_idx < 0) {
      return {};
    }
    return [select_idx, session](std::unique_ptr<MuxEvent>&& event) -> asio::awaitable<bool> {
      if (select_idx >= session->conns.size()) {
        co_return false;
      }
      auto write_conn = session->conns[select_idx];
      if (write_conn) {
        co_return co_await write_conn->Write(std::move(event));
      }
      co_return false;
    };
  };
  return f;
}
std::shared_ptr<MuxConnManager>& MuxConnManager::GetInstance() {
  static std::shared_ptr<MuxConnManager> g_instance = std::make_shared<MuxConnManager>();
  return g_instance;
}

void MuxConnManager::RegisterStat() {
  register_stat_func([]() -> StatValues {
    StatValues vals;
    auto& kv = vals["Mux"];
    kv["stream_num"] = std::to_string(MuxStream::Size());
    kv["stream_active_num"] = std::to_string(MuxStream::ActiveSize());
    kv["connection_num"] = std::to_string(MuxConnection::Size());
    kv["connection_active_num"] = std::to_string(MuxConnection::ActiveSize());
    return vals;
  });
  register_stat_func([]() -> StatValues {
    StatValues vals;
    MuxConnManager::GetInstance()->ReportStatInfo(vals);
    return vals;
  });
}

void MuxConnManager::ReportStatInfo(StatValues& stats) {
  for (const auto& [user, user_conn] : mux_conns_) {
    for (size_t i = 0; i < kMuxConnTypeNum; i++) {
      auto& session_table = user_conn->sessions[i];
      if (session_table.empty()) {
        continue;
      }
      std::string_view type = (i == 0 ? "ENTRY" : (i == 1 ? "EXIT" : "MIDDLE"));
      for (const auto& [client_id, session] : session_table) {
        auto& kv = stats[fmt::format("[{}]MuxConnection:{}:{}", type, user, client_id)];
        session->ReportStatInfo(kv);
      }
    }
  }
}
EventWriterFactory MuxConnManager::GetRelayEventWriterFactory(std::string_view user,
                                                              uint64_t* client_id) {
  auto user_found = mux_conns_.find(user);
  if (user_found == mux_conns_.end()) {
    return {};
  }
  UserMuxConnPtr user_conn = user_found->second;
  auto& exit_session_table = user_conn->sessions[MUX_EXIT_CONN];
  for (auto& [cid, session] : exit_session_table) {
    if (session->conns.size() == 0) {
      continue;
    }
    if (nullptr != client_id) {
      *client_id = cid;
    }
    return session->GetEventWriterFactory();
  }
  return {};
}

EventWriterFactory MuxConnManager::GetEventWriterFactory(std::string_view user, uint64_t client_id,
                                                         MuxConnectionType type) {
  auto user_found = mux_conns_.find(user);
  if (user_found == mux_conns_.end()) {
    return {};
  }
  UserMuxConnPtr user_conn = user_found->second;
  auto found = user_conn->sessions[type].find(client_id);
  if (found == user_conn->sessions[type].end()) {
    return {};
  }
  return found->second->GetEventWriterFactory();
}

UserMuxConnPtr MuxConnManager::GetUserMuxConn(std::string_view user) {
  UserMuxConnPtr user_conn;
  auto found = mux_conns_.find(user);
  if (found == mux_conns_.end()) {
    user_conn = std::make_shared<UserMuxConn>();
    user_conn->user = std::string(user.data(), user.size());
    std::string_view user_view = user_conn->user;
    mux_conns_.emplace(user_view, user_conn);
  } else {
    user_conn = found->second;
  }
  return user_conn;
}

MuxSessionPtr MuxConnManager::GetSession(std::string_view user, uint64_t client_id,
                                         MuxConnectionType type) {
  UserMuxConnPtr user_conn = GetUserMuxConn(user);
  MuxSessionPtr& session = user_conn->sessions[type][client_id];
  if (!session) {
    session = std::make_shared<MuxSession>();
  }
  return session;
}

MuxSessionPtr MuxConnManager::Add(std::string_view user, uint64_t client_id, MuxConnectionPtr conn,
                                  uint32_t* conn_idx) {
  UserMuxConnPtr user_conn = GetUserMuxConn(user);
  MuxSessionPtr& session = user_conn->sessions[conn->GetType()][client_id];
  if (!session) {
    session = std::make_shared<MuxSession>();
  }
  size_t idx = 0;
  for (auto& c : session->conns) {
    if (!c) {
      c = conn;
      if (nullptr != conn_idx) {
        *conn_idx = idx;
      }
      return session;
    }
    idx++;
  }
  session->conns.emplace_back(conn);
  if (nullptr != conn_idx) {
    *conn_idx = session->conns.size() - 1;
  }
  return session;
}
void MuxConnManager::Remove(std::string_view user, uint64_t client_id, MuxConnection* conn) {
  auto user_found = mux_conns_.find(user);
  if (user_found == mux_conns_.end()) {
    return;
  }

  auto found = user_found->second->sessions[conn->GetType()].find(client_id);
  if (found == user_found->second->sessions[conn->GetType()].end()) {
    return;
  }
  bool all_empty = true;
  bool match = false;
  MuxSessionPtr& session = found->second;
  for (auto& c : session->conns) {
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
    user_found->second->sessions[conn->GetType()].erase(found);
  }
}
void MuxConnManager::Remove(std::string_view user, uint64_t client_id, MuxConnectionPtr conn) {
  Remove(user, client_id, conn.get());
}
}  // namespace snova
