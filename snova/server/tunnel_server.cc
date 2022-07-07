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
#include "snova/server/tunnel_server.h"

#include <memory>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "asio/experimental/as_tuple.hpp"
#include "snova/log/log_macros.h"
#include "snova/mux/mux_conn_manager.h"
#include "snova/server/relay.h"
#include "snova/util/flags.h"
namespace snova {

using TunnelServerTable = absl::flat_hash_map<uint64_t, TCPServerSocketPtr>;
static TunnelServerTable g_tunnel_servers;
static uint64_t g_tunnel_server_id_seed = 1;

void close_tunnel_server(uint32_t server_id) {
  auto found = g_tunnel_servers.find(server_id);
  if (found != g_tunnel_servers.end()) {
    TCPServerSocketPtr server = found->second;
    server->cancel();
    server->close();
    g_tunnel_servers.erase(found);
  }
}

static ::asio::awaitable<void> handle_conn(::asio::ip::tcp::socket sock,
                                           const NetAddress& dst_addr) {
  RelayContext relay_ctx;
  relay_ctx.user = GlobalFlags::GetIntance()->GetUser();
  relay_ctx.remote_host = dst_addr.host;
  relay_ctx.remote_port = dst_addr.port;
  relay_ctx.is_tcp = true;
  co_await relay(std::move(sock), Bytes{}, relay_ctx);
}

static ::asio::awaitable<void> server_loop(TCPServerSocketPtr server, NetAddress dst_addr) {
  while (true) {
    auto [ec, client] =
        co_await server->async_accept(::asio::experimental::as_tuple(::asio::use_awaitable));
    if (ec) {
      SNOVA_ERROR("Failed to accept with error:{}", ec.message());
      co_return;
    }
    // SNOVA_INFO("Receive new connection.");
    auto ex = co_await asio::this_coro::executor;
    ::asio::co_spawn(ex, handle_conn(std::move(client), dst_addr), ::asio::detached);
  }
  co_return;
}

asio::awaitable<TunnelServerListenResult> start_tunnel_server(const NetAddress& listen_addr,
                                                              const NetAddress& dst_addr) {
  SNOVA_INFO("Start tunnel server listen on address [{}].", listen_addr.String());
  auto ex = co_await asio::this_coro::executor;
  ::asio::ip::tcp::endpoint endpoint;
  auto resolve_ec = co_await listen_addr.GetEndpoint(&endpoint);
  if (resolve_ec) {
    co_return TunnelServerListenResult{0, resolve_ec};
  }
  ::asio::ip::tcp::acceptor acceptor(ex);
  acceptor.open(endpoint.protocol());
  acceptor.set_option(::asio::socket_base::reuse_address(true));
  std::error_code ec;
  acceptor.bind(endpoint, ec);
  if (ec) {
    SNOVA_ERROR("Failed to bind {} with error:{}", listen_addr.String(), ec.message());
    co_return TunnelServerListenResult{0, ec};
  }
  acceptor.listen(::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    SNOVA_ERROR("Failed to listen {} with error:{}", listen_addr.String(), ec.message());
    co_return TunnelServerListenResult{0, ec};
  }
  TCPServerSocketPtr server_socket = std::make_shared<TCPServerSocket>(std::move(acceptor));
  uint64_t server_id = g_tunnel_server_id_seed;
  g_tunnel_server_id_seed++;
  g_tunnel_servers[server_id] = server_socket;
  NetAddress tunnel_dst_addr = dst_addr;
  ::asio::co_spawn(ex, server_loop(server_socket, std::move(tunnel_dst_addr)), ::asio::detached);
  co_return TunnelServerListenResult{server_id, std::error_code{}};
}

asio::awaitable<std::unique_ptr<MuxEvent>> tunnel_server_handler(
    const std::string& user, uint64_t client_id, std::unique_ptr<MuxEvent>&& event) {
  std::unique_ptr<MuxEvent> mux_event =
      std::move(event);  // this make rvalue event not release after co_await
  auto ex = co_await asio::this_coro::executor;
  std::unique_ptr<TunnelOpenResponse> tunnel_rsp = std::make_unique<TunnelOpenResponse>();
  TunnelOpenRequest* tunnel_request = dynamic_cast<TunnelOpenRequest*>(mux_event.get());
  if (nullptr == tunnel_request) {
    tunnel_rsp->event.success = false;
    tunnel_rsp->event.errc = ERR_INVALID_EVENT;
    SNOVA_ERROR("null request for EVENT_TUNNEL_REQ");
    co_return std::move(tunnel_rsp);
  }
  if (g_is_entry_node) {
    MuxSessionPtr session =
        MuxConnManager::GetInstance()->GetSession(user, client_id, MUX_EXIT_CONN);
    if (!session) {
      SNOVA_ERROR("Empty mux session to start tunnel server.");
      tunnel_rsp->event.success = false;
      tunnel_rsp->event.errc = ERR_EMPTY_MUX_SESSION;
    } else {
      // start tunnel server
      NetAddress server_addr, dst_addr;
      server_addr.host = "0.0.0.0";
      server_addr.port = tunnel_request->event.remote_port;
      dst_addr.host = tunnel_request->event.local_host;
      dst_addr.port = tunnel_request->event.local_port;
      auto [server_id, ec] = co_await start_tunnel_server(server_addr, dst_addr);
      if (!ec) {
        tunnel_rsp->event.success = true;
        tunnel_rsp->event.tunnel_id = server_id;
        session->tunnel_servers.emplace_back(server_id);
      } else {
        SNOVA_ERROR("Start tunnel server failed:{}", ec);
        tunnel_rsp->event.success = false;
        tunnel_rsp->event.errc = ec.value();
      }
    }

  } else {
    tunnel_rsp->event.success = false;
    tunnel_rsp->event.errc = ERR_INVALID_EVENT;
    SNOVA_ERROR("Invalid role to handle EVENT_TUNNEL_REQ");
  }
  co_return std::move(tunnel_rsp);
}

}  // namespace snova
