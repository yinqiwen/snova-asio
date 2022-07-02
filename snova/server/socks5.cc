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
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "asio/experimental/as_tuple.hpp"
#include "snova/log/log_macros.h"
#include "snova/server/entry_server.h"
#include "snova/server/relay.h"

namespace snova {
static constexpr uint8_t kMethodNoAuth = 0;
static constexpr uint8_t kMethodGSSAPI = 1;
static constexpr uint8_t kCmdConnect = 1;
static constexpr uint8_t kAddrIPV4 = 1;
static constexpr uint8_t kAddrIPV6 = 4;
static constexpr uint8_t kAddrDomain = 3;
asio::awaitable<void> handle_socks5_connection(::asio::ip::tcp::socket&& s, IOBufPtr&& rbuf,
                                               const Bytes& readable_data) {
  // SNOVA_INFO("Handle proxy connection by socks5.");
  ::asio::ip::tcp::socket sock(std::move(s));  //  make rvalue sock not release after co_await
  IOBufPtr conn_read_buffer = std::move(rbuf);
  IOBuf& read_buffer = *conn_read_buffer;
  uint8_t num_methods = readable_data.data()[1];
  absl::string_view methods_view((const char*)(readable_data.data() + 2), num_methods);

  if (methods_view.find(kMethodNoAuth) == absl::string_view::npos) {
    SNOVA_ERROR("Only support NOAuth socks5 client.");
    co_return;
  }
  uint8_t method_res[2];
  method_res[0] = 5;  // socks5
  method_res[1] = kMethodNoAuth;
  co_await ::asio::async_write(sock, ::asio::buffer(method_res, 2),
                               ::asio::experimental::as_tuple(::asio::use_awaitable));
  auto [ec, n] =
      co_await sock.async_read_some(::asio::buffer(read_buffer.data(), read_buffer.size()),
                                    ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (ec || n < 4) {
    SNOVA_ERROR("Read socks5 client failed with error:{}, count:{}", ec, n);
    co_return;
  }

  if (read_buffer[0] != 5) {
    SNOVA_ERROR("Socks5 not confirm with socks5");
    co_return;
  }
  if (read_buffer[1] != kCmdConnect) {
    SNOVA_ERROR("Only connect command supported.");
    co_return;
  }
  uint8_t target_addr_type = read_buffer[3];
  uint16_t remote_port = 0;
  remote_port = read_buffer[n - 2];
  remote_port = ((remote_port << 8) + read_buffer[n - 1]);
  std::string remote_host;
  std::unique_ptr<::asio::ip::tcp::endpoint> remote_endpoint;
  switch (target_addr_type) {
    case kAddrIPV4: {
      if (n != 10) {
        SNOVA_ERROR("Missing data for socks5 connection with addr type:{}", kAddrIPV4);
        co_return;
      }
      std::array<uint8_t, 4> v4_ip = {read_buffer[4], read_buffer[5], read_buffer[6],
                                      read_buffer[7]};
      remote_endpoint = std::make_unique<::asio::ip::tcp::endpoint>(
          asio::ip::make_address_v4(v4_ip), remote_port);
      remote_host = remote_endpoint->address().to_string();
      break;
    }
    case kAddrIPV6: {
      if (n != 22) {
        SNOVA_ERROR("Missing data for socks5 connection with addr type:{}", kAddrIPV6);
        co_return;
      }
      std::array<uint8_t, 16> v6_ip = {
          read_buffer[4],  read_buffer[5],  read_buffer[6],  read_buffer[7],
          read_buffer[8],  read_buffer[9],  read_buffer[10], read_buffer[11],
          read_buffer[12], read_buffer[13], read_buffer[14], read_buffer[15],
          read_buffer[16], read_buffer[17], read_buffer[18], read_buffer[19]};
      remote_endpoint = std::make_unique<::asio::ip::tcp::endpoint>(
          asio::ip::make_address_v6(v6_ip), remote_port);
      remote_host = remote_endpoint->address().to_string();
      break;
    }
    case kAddrDomain: {
      uint8_t domain_len = read_buffer[4];
      if (n != (7 + domain_len)) {
        SNOVA_ERROR("Missing data for socks5 connection with addr type:{}", kAddrDomain);
        co_return;
      }
      remote_host.assign((const char*)(read_buffer.data() + 5), domain_len);
      break;
    }
    default: {
      SNOVA_ERROR("Unsupported addr type:{}.", target_addr_type);
      co_return;
    }
  }
  SNOVA_INFO("Socks5 target {}:{}", remote_host, remote_port);
  uint8_t socks5_resp[10];
  memset(socks5_resp, 0, 10);
  socks5_resp[0] = 5;
  socks5_resp[1] = 0;  // SOCKS_RESP_SUUCESS
  socks5_resp[2] = 0;
  socks5_resp[3] = 1;  // socksAtypeV4         = 0x01
  co_await ::asio::async_write(sock, ::asio::buffer(socks5_resp, 10),
                               ::asio::experimental::as_tuple(::asio::use_awaitable));

  auto [rec, rn] =
      co_await sock.async_read_some(::asio::buffer(read_buffer.data(), read_buffer.size()),
                                    ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (rec) {
    SNOVA_ERROR("Failed to read local connection with error:{}", rec);
    co_return;
  }
  if (rn > 3 && read_buffer[0] == 0x16) {  // tls
    auto tls_major_ver = read_buffer[1];
    if (tls_major_ver < 3) {
      // no SNI before sslv3
      SNOVA_ERROR("sslv2/sslv1 not supported!");
      co_return;
    }
    Bytes client_hello(read_buffer.data(), rn);
    bool success = co_await handle_tls_connection(std::move(sock), std::move(conn_read_buffer),
                                                  client_hello, std::move(remote_endpoint));
    if (success) {
      co_return;
    }
  }

  RelayContext relay_ctx;
  relay_ctx.remote_host = std::move(remote_host);
  relay_ctx.remote_port = remote_port;
  relay_ctx.is_tcp = true;
  co_await relay(std::move(sock), Bytes{}, relay_ctx);
}
}  // namespace snova
