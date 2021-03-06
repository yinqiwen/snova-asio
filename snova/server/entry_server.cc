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
#include <utility>

#include "absl/cleanup/cleanup.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "asio/experimental/as_tuple.hpp"
#include "snova/log/log_macros.h"
#include "snova/server/relay.h"
#include "snova/util/address.h"
#include "snova/util/flags.h"
#include "snova/util/net_helper.h"
#include "snova/util/stat.h"

namespace snova {
static uint32_t g_local_proxy_conn_num = 0;

static ::asio::awaitable<void> handle_conn(::asio::ip::tcp::socket sock) {
  g_local_proxy_conn_num++;
  absl::Cleanup source_closer = [] { g_local_proxy_conn_num--; };
  std::unique_ptr<::asio::ip::tcp::endpoint> remote_endpoint;
  if (g_is_redirect_node) {
    remote_endpoint = std::make_unique<::asio::ip::tcp::endpoint>();
    if (0 != get_orig_dst(sock.native_handle(), remote_endpoint.get())) {
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
    bool success = co_await handle_tls_connection(std::move(sock), std::move(buffer), readable,
                                                  std::move(remote_endpoint));
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
    RelayContext relay_ctx;
    relay_ctx.remote_host = remote_endpoint->address().to_string();
    relay_ctx.remote_port = remote_endpoint->port();
    relay_ctx.is_tcp = true;
    co_await relay_direct(std::move(sock), readable, relay_ctx);
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

    if (g_entry_socket_send_buffer_size > 0) {
      ::asio::socket_base::send_buffer_size current_send_buffer_size;
      client.get_option(current_send_buffer_size);
      if (g_entry_socket_send_buffer_size < current_send_buffer_size.value()) {
        ::asio::socket_base::send_buffer_size new_send_buffer_size(g_entry_socket_send_buffer_size);
        client.set_option(new_send_buffer_size);
      }
    }

    if (g_entry_socket_recv_buffer_size > 0) {
      ::asio::socket_base::receive_buffer_size curent_recv_buffer_size;
      client.get_option(curent_recv_buffer_size);
      if (g_entry_socket_recv_buffer_size < curent_recv_buffer_size.value()) {
        ::asio::socket_base::receive_buffer_size new_recv_buffer_size(
            g_entry_socket_recv_buffer_size);
        client.set_option(new_recv_buffer_size);
      }
    }

    auto ex = co_await asio::this_coro::executor;
    ::asio::co_spawn(ex, handle_conn(std::move(client)), ::asio::detached);
  }
  co_return;
}

asio::awaitable<std::error_code> start_entry_server(const NetAddress& server_address) {
  register_stat_func([]() -> StatValues {
    StatValues vals;
    auto& kv = vals["LocalServer"];
    kv["local_proxy_conn_num"] = std::to_string(g_local_proxy_conn_num);
    return vals;
  });

  auto ex = co_await asio::this_coro::executor;
  ::asio::ip::tcp::endpoint endpoint;
  auto resolve_ec = co_await server_address.GetEndpoint(&endpoint);
  if (resolve_ec) {
    co_return resolve_ec;
  }
  ::asio::ip::tcp::acceptor acceptor(ex);
  acceptor.open(endpoint.protocol());
  acceptor.set_option(::asio::socket_base::reuse_address(true));
  std::error_code ec;
  acceptor.bind(endpoint, ec);
  if (ec) {
    SNOVA_ERROR("Failed to bind {} with error:{}", server_address.String(), ec.message());
    co_return ec;
  }
  acceptor.listen(::asio::socket_base::max_listen_connections, ec);
  if (ec) {
    SNOVA_ERROR("Failed to listen {} with error:{}", server_address.String(), ec.message());
    co_return ec;
  }
  SNOVA_INFO("Start listen on address:{}.", server_address.String());
  ::asio::co_spawn(ex, server_loop(std::move(acceptor)), ::asio::detached);
  co_return ec;
}
}  // namespace snova
