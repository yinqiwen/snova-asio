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
#include "snova/mux/mux_client.h"
#include <memory>

#include "asio/experimental/as_tuple.hpp"
#include "snova/log/log_macros.h"
#include "snova/util/flags.h"
#include "snova/util/net_helper.h"
#include "snova/util/time_wheel.h"
#include "spdlog/fmt/fmt.h"

namespace snova {
std::shared_ptr<MuxClient>& MuxClient::GetInstance() {
  static std::shared_ptr<MuxClient> g_instance = std::make_shared<MuxClient>();
  return g_instance;
}

asio::awaitable<std::error_code> MuxClient::NewConnection(uint32_t idx) {
  if (idx >= remote_session_->conns.size()) {
    co_return std::make_error_code(std::errc::invalid_argument);
  }
  auto ex = co_await asio::this_coro::executor;
  ::asio::ip::tcp::socket socket(ex);
  if (g_http_proxy_host.empty()) {
    auto [ec] = co_await socket.async_connect(
        remote_endpoint_, ::asio::experimental::as_tuple(::asio::use_awaitable));
    if (ec) {
      SNOVA_ERROR("Failed to connect:{} with error:{}", remote_endpoint_, ec);
      co_return ec;
    }
  } else {
    auto ec = co_await connect_remote_via_http_proxy(socket, remote_endpoint_, g_http_proxy_host,
                                                     g_http_proxy_port);
    if (ec) {
      SNOVA_ERROR("Failed to connect:{} via http proxy with error:{}", remote_endpoint_, ec);
      co_return ec;
    }
  }

  std::unique_ptr<CipherContext> cipher_ctx = CipherContext::New(cipher_method_, cipher_key_);
  MuxConnectionPtr conn =
      std::make_shared<MuxConnection>(std::move(socket), std::move(cipher_ctx), true);
  bool auth_success = co_await conn->ClientAuth(auth_user_, client_id_);
  if (!auth_success) {
    co_return std::make_error_code(std::errc::invalid_argument);
  }
  conn->SetIdx(idx);
  SNOVA_INFO("[{}]Success to connect:{}", idx, remote_endpoint_);
  ::asio::co_spawn(
      ex,
      [this, idx, conn]() -> asio::awaitable<void> {
        co_await conn->ReadEventLoop();
        if (!conn->IsRetired()) {
          remote_session_->conns[idx] = nullptr;
        }
        co_return;
      },
      ::asio::detached);
  remote_session_->conns[idx] = conn;
  co_return std::error_code{};
}
asio::awaitable<std::error_code> MuxClient::Init(const std::string& user,
                                                 const std::string& cipher_method,
                                                 const std::string& cipher_key) {
  // remote_conns_.resize(g_conn_num_per_server);
  remote_session_ = MuxConnManager::GetInstance()->GetSession(auth_user_, client_id_);
  remote_session_->conns.resize(g_conn_num_per_server);
  PaserEndpointResult parse_result = parse_endpoint(g_remote_server);
  if (parse_result.second) {
    co_return parse_result.second;
  }
  std::unique_ptr<CipherContext> cipher_ctx = CipherContext::New(cipher_method, cipher_key);
  if (!cipher_ctx) {
    SNOVA_ERROR("Invalid cipher_method:{} cipher_key:{}", cipher_method, cipher_key);
    co_return std::make_error_code(std::errc::invalid_argument);
  }
  auth_user_ = user;
  cipher_method_ = cipher_method;
  cipher_key_ = cipher_key;

  remote_endpoint_ = *parse_result.first;
  auto ec = co_await NewConnection(0);
  if (ec) {
    co_return ec;
  }

  for (uint32_t i = 1; i < g_conn_num_per_server; i++) {
    co_await NewConnection(i);
  }
  auto ex = co_await asio::this_coro::executor;
  ::asio::co_spawn(ex, CheckConnections(), ::asio::detached);
  co_return std::error_code{};
}
asio::awaitable<void> MuxClient::CheckConnections() {
  auto ex = co_await asio::this_coro::executor;
  ::asio::steady_timer timer(ex);
  std::chrono::milliseconds period(3000);
  while (true) {
    timer.expires_after(period);
    co_await timer.async_wait(::asio::experimental::as_tuple(::asio::use_awaitable));
    for (uint32_t i = 0; i < g_conn_num_per_server; i++) {
      auto& conn = remote_session_->conns[i];
      if (!conn) {
        co_await NewConnection(i);
      } else {
        uint32_t now = time(nullptr);
        conn->ResetCounter(now);
        if (now > conn->GetExpireAtUnixSecs()) {
          SNOVA_INFO("[{}]Try to retire connection since now:{}, retire time:{}", i, now,
                     conn->GetExpireAtUnixSecs());
          auto retired_conn = conn;
          retired_conn->SetRetired();
          conn = nullptr;
          auto retire_event = std::make_unique<RetireConnRequest>();
          co_await retired_conn->WriteEvent(std::move(retire_event));
          auto timeout_callback = [retired_conn]() -> asio::awaitable<void> {
            SNOVA_ERROR("[{}]Close retired connection since it's not active since {}s ago.",
                        retired_conn->GetIdx(),
                        time(nullptr) - retired_conn->GetLastActiveUnixSecs());
            retired_conn->Close();
            co_return;
          };
          auto get_active_time = [retired_conn]() -> uint32_t {
            return retired_conn->GetLastActiveUnixSecs();
          };
          TimeWheel::GetInstance()->Add(std::move(timeout_callback), std::move(get_active_time),
                                        15);
        }
      }
    }
  }
  co_return;
}

void MuxClient::SetClientId(uint64_t client_id) { client_id_ = client_id; }

EventWriterFactory MuxClient::GetEventWriterFactory() {
  return remote_session_->GetEventWriterFactory();
}

void MuxClient::ReportStatInfo(StatValues& stats) {
  if (!remote_session_) {
    return;
  }
  auto& kv = stats["MuxClient"];
  uint32_t now = time(nullptr);
  for (size_t i = 0; i < remote_session_->conns.size(); i++) {
    auto& conn = remote_session_->conns[i];
    if (!conn) {
      kv[fmt::format("[{}]", i)] = "NULL";
    } else {
      kv[fmt::format("[{}]read_state", i)] = conn->GetReadState();
      kv[fmt::format("[{}]inactive_secs", i)] = std::to_string(now - conn->GetLastActiveUnixSecs());
      kv[fmt::format("[{}]recv_bytes", i)] = std::to_string(conn->GetRecvBytes());
      kv[fmt::format("[{}]send_bytes", i)] = std::to_string(conn->GetSendBytes());
      kv[fmt::format("[{}]latest_30s_recv_bytes", i)] =
          std::to_string(conn->GetLatestWindowRecvBytes());
      kv[fmt::format("[{}]latest_30s_send_bytes", i)] =
          std::to_string(conn->GetLatestWindowSendBytes());
    }
  }
}

void register_mux_stat() {
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
    MuxClient::GetInstance()->ReportStatInfo(vals);
    return vals;
  });
}

}  // namespace snova
