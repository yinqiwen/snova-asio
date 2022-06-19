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
#include "snova/server/entry_server.h"
#include <string_view>
#include <system_error>

#include "absl/cleanup/cleanup.h"
#include "absl/flags/declare.h"
#include "absl/flags/flag.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "asio/experimental/as_tuple.hpp"
#include "snova/log/log_macros.h"
#include "snova/server/relay.h"
#include "snova/util/flags.h"
#include "snova/util/net_helper.h"
#include "snova/util/stat.h"
#include "spdlog/fmt/bundled/ostream.h"

namespace snova {
static uint32_t g_local_proxy_conn_num = 0;

static ::asio::awaitable<void> handle_conn(::asio::ip::tcp::socket sock) {
  g_local_proxy_conn_num++;
  absl::Cleanup source_closer = [] { g_local_proxy_conn_num--; };
  std::unique_ptr<::asio::ip::tcp::endpoint> remote_endpoint;
  if (g_is_entry_node && g_is_redirect_node) {
    remote_endpoint = std::make_unique<::asio::ip::tcp::endpoint>();
    if (0 != get_orig_dst(sock.native_handle(), *remote_endpoint)) {
      remote_endpoint.release();
    } else {
      if (is_private_address(remote_endpoint->address())) {
        remote_endpoint.release();
      }
    }
  }
  bool has_redirect_address = (remote_endpoint != nullptr);

  IOBufPtr buffer = get_iobuf(kMaxChunkSize);
  auto [ec, n] =
      co_await sock.async_read_some(::asio::buffer(buffer->data(), kMaxChunkSize),
                                    ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (ec) {
    co_return;
  }
  if (0 == n) {
    co_return;
  }
  if (n < 3) {
    SNOVA_ERROR("no enought data received.");
    co_return;
  }
  Bytes readable(buffer->data(), n);
  absl::string_view cmd3((const char*)(readable.data()), 3);
  if (!has_redirect_address && (*buffer)[0] == 5) {  // socks5
    co_await handle_socks5_connection(std::move(sock), std::move(buffer), readable);
  } else if (!has_redirect_address && (*buffer)[0] == 4) {  // socks4
    SNOVA_ERROR("Socks4 not supported!");
    co_return;
  } else if ((*buffer)[0] == 0x16) {  // tls
    auto tls_major_ver = (*buffer)[1];
    if (tls_major_ver < 3) {
      // no SNI before sslv3
      SNOVA_ERROR("sslv2/sslv1 not supported!");
      co_return;
    }
    bool success = co_await handle_tls_connection(std::move(sock), std::move(buffer), readable);
    if (success) {
      co_return;
    }
  } else if (absl::EqualsIgnoreCase(cmd3, "GET") || absl::EqualsIgnoreCase(cmd3, "CON") ||
             absl::EqualsIgnoreCase(cmd3, "PUT") || absl::EqualsIgnoreCase(cmd3, "POS") ||
             absl::EqualsIgnoreCase(cmd3, "DEL") || absl::EqualsIgnoreCase(cmd3, "OPT") ||
             absl::EqualsIgnoreCase(cmd3, "TRA") || absl::EqualsIgnoreCase(cmd3, "PAT") ||
             absl::EqualsIgnoreCase(cmd3, "HEA") || absl::EqualsIgnoreCase(cmd3, "UPG")) {
    co_await handle_http_connection(std::move(sock), std::move(buffer), readable);
    co_return;
  }

  // other
  if (remote_endpoint) {
    // just relay to direct
    SNOVA_INFO("Redirect proxy connection to {}.", *remote_endpoint);
    co_await relay_direct(std::move(sock), readable, remote_endpoint->address().to_string(),
                          remote_endpoint->port(), true);
  } else {
    // no remote host&port to relay
  }
  co_return;
}

static ::asio::awaitable<void> server_loop(::asio::ip::tcp::acceptor server) {
  while (true) {
    auto [ec, client] =
        co_await server.async_accept(::asio::experimental::as_tuple(::asio::use_awaitable));
    if (ec) {
      SNOVA_ERROR("Failed to accept with error:{}", ec.message());
      co_return;
    }
    // SNOVA_INFO("Receive new local connection.");
    auto ex = co_await asio::this_coro::executor;
    ::asio::co_spawn(ex, handle_conn(std::move(client)), ::asio::detached);
  }
  co_return;
}

asio::awaitable<std::error_code> start_entry_server(const std::string& addr) {
  PaserEndpointResult parse_result = parse_endpoint(addr);
  if (parse_result.second) {
    co_return parse_result.second;
  }
  register_stat_func([]() -> StatValues {
    StatValues vals;
    auto& kv = vals["LocalServer"];
    kv["local_proxy_conn_num"] = std::to_string(g_local_proxy_conn_num);
    return vals;
  });

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
  SNOVA_INFO("Start listen on address:{}.", addr);
  ::asio::co_spawn(ex, server_loop(std::move(acceptor)), ::asio::detached);
  co_return ec;
}
}  // namespace snova
