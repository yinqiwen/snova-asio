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
#include "snova/server/mux_server.h"
#include <string_view>
#include <system_error>

#include "absl/cleanup/cleanup.h"
#include "absl/strings/str_split.h"
#include "asio/experimental/as_tuple.hpp"
#include "snova/log/log_macros.h"
#include "snova/mux/mux_conn_manager.h"
#include "snova/mux/mux_connection.h"
#include "snova/server/relay.h"
#include "snova/util/net_helper.h"
#include "snova/util/stat.h"
#include "snova/util/time_wheel.h"

namespace snova {
static uint32_t g_remote_server_conn_num = 0;
static ::asio::awaitable<void> handle_conn(::asio::ip::tcp::socket sock,
                                           const std::string& cipher_method,
                                           const std::string& cipher_key) {
  g_remote_server_conn_num++;
  absl::Cleanup auto_counter = [] { g_remote_server_conn_num--; };
  std::unique_ptr<CipherContext> cipher_ctx = CipherContext::New(cipher_method, cipher_key);
  MuxConnectionPtr mux_conn =
      std::make_shared<MuxConnection>(std::move(sock), std::move(cipher_ctx), false);
  auto [auth_user, client_id, auth_success] = co_await mux_conn->ServerAuth();
  if (!auth_success) {
    SNOVA_ERROR("Server auth failed!");
    co_return;
  }
  mux_conn->SetRelayHandler(relay_handler);
  std::string mux_user = std::move(auth_user);
  mux_conn->SetRetireCallback([mux_user](MuxConnection* c) {
    SNOVA_INFO("[{}]Connection retired!", c->GetIdx());
    MuxConnManager::GetInstance()->Remove(mux_user, c->GetClientId(), c);
    auto retired_conn = c->GetShared();
    TimeWheel::GetInstance()->Add(
        [retired_conn]() -> asio::awaitable<void> {
          SNOVA_ERROR("[{}]Close retired connection since it's not active since {}s ago.",
                      retired_conn->GetIdx(),
                      time(nullptr) - retired_conn->GetLastActiveUnixSecs());
          retired_conn->Close();
          co_return;
        },
        [retired_conn]() -> uint32_t { return retired_conn->GetLastActiveUnixSecs(); }, 60);
  });
  uint32_t idx = MuxConnManager::GetInstance()->Add(mux_user, client_id, mux_conn);
  mux_conn->SetIdx(idx);
  auto ex = co_await asio::this_coro::executor;
  co_await mux_conn->ReadEventLoop();
  mux_conn->Close();
  MuxConnManager::GetInstance()->Remove(mux_user, client_id, mux_conn);
}

static ::asio::awaitable<void> server_loop(::asio::ip::tcp::acceptor server,
                                           const std::string& cipher_method,
                                           const std::string& cipher_key) {
  while (true) {
    auto [ec, client] =
        co_await server.async_accept(::asio::experimental::as_tuple(::asio::use_awaitable));
    if (ec) {
      SNOVA_ERROR("Failed to accept with error:{}", ec.message());
      co_return;
    }
    SNOVA_INFO("Receive new connection.");
    auto ex = co_await asio::this_coro::executor;
    ::asio::co_spawn(ex, handle_conn(std::move(client), cipher_method, cipher_key),
                     ::asio::detached);
  }
  co_return;
}

asio::awaitable<std::error_code> start_mux_server(const std::string& addr,
                                                  const std::string& cipher_method,
                                                  const std::string& cipher_key) {
  SNOVA_INFO("Start listen on address [{}] with cipher_method:{}", addr, cipher_method);
  register_stat_func([]() -> StatValues {
    StatValues vals;
    auto& kv = vals["RemoteServer"];
    kv["remote_server_conn_num"] = std::to_string(g_remote_server_conn_num);
    return vals;
  });
  std::unique_ptr<CipherContext> cipher_ctx = CipherContext::New(cipher_method, cipher_key);
  if (!cipher_ctx) {
    co_return std::make_error_code(std::errc::invalid_argument);
  }
  PaserEndpointResult parse_result = parse_endpoint(addr);
  if (parse_result.second) {
    co_return parse_result.second;
  }
  auto ex = co_await asio::this_coro::executor;
  ::asio::ip::tcp::endpoint& endpoint = *parse_result.first;
  ::asio::ip::tcp::acceptor acceptor(ex);
  acceptor.open(endpoint.protocol());
  acceptor.set_option(::asio::socket_base::reuse_address(true));
  std::error_code ec;
  acceptor.bind(endpoint, ec);
  if (ec) {
    SNOVA_ERROR("Failed to bind {} with error:{}", addr, ec.message());
    co_return ec;
  }
  acceptor.listen(::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    SNOVA_ERROR("Failed to listen {} with error:{}", addr, ec.message());
    co_return ec;
  }

  ::asio::co_spawn(ex, server_loop(std::move(acceptor), cipher_method, cipher_key),
                   ::asio::detached);
  co_return ec;
}
}  // namespace snova
