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
#include "snova/server/dns_proxy_server.h"
#include <chrono>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/strings/escaping.h"
#include "absl/strings/match.h"
#include "absl/strings/numbers.h"
#include "asio/experimental/as_tuple.hpp"
#include "snova/io/io_util.h"
#include "snova/io/tls_socket.h"
#include "snova/log/log_macros.h"
#include "snova/util/endian.h"
#include "snova/util/flags.h"
#include "snova/util/http_helper.h"
#include "snova/util/net_helper.h"
#include "snova/util/time_wheel.h"

namespace snova {
using std::chrono::milliseconds;
struct DNSProxyServer {
  UDPSocketPtr server;
  UDPSocketPtr default_ns;
  std::shared_ptr<TlsSocket> trusted_ns;
  bool is_trusted_ns_https = false;
};

struct DNSProxyState {
  ::asio::ip::udp::endpoint orig_endpoint;
  std::vector<uint8_t> orig_payload;
  uint64_t init_mstime = 0;
  CancelFunc cancel_ns_timeout;
  bool disable_default_ns = false;
  bool disable_trusted_ns = false;
};

using DNSProxyStateTable = absl::flat_hash_map<uint16_t, DNSProxyState>;
static DNSProxyStateTable g_dns_states;

static asio::awaitable<void> dns_response(UDPSocketPtr& server, uint16_t txid,
                                          const uint8_t* payload, uint32_t payload_len) {
  auto found = g_dns_states.find(txid);
  if (found == g_dns_states.end()) {
    SNOVA_ERROR("No '{}' found in dns proxy table.", txid);
    co_return;
  }
  DNSProxyState& state = found->second;
  state.cancel_ns_timeout();
  auto orig_endpoint = state.orig_endpoint;
  auto now =
      std::chrono::duration_cast<milliseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
  SNOVA_INFO("Cost {}ms for dns query:{}", now - state.init_mstime, txid);
  g_dns_states.erase(found);
  co_await server->async_send_to(::asio::buffer(payload, payload_len), orig_endpoint,
                                 ::asio::experimental::as_tuple(::asio::use_awaitable));
}

static asio::awaitable<std::error_code> dns_over_trusted(std::shared_ptr<DNSProxyServer>& server,
                                                         const DNSOptions& options,
                                                         const uint8_t* payload,
                                                         size_t payload_len) {
  if (!server->trusted_ns) {
    SNOVA_ERROR("null trusted ns connection.");
    co_return std::error_code{};
  }
  SNOVA_INFO("DNS over trusted ns:{}/{}", options.trusted_ns->host, options.trusted_ns->port);
  if (options.trusted_ns->schema == "dot") {
    // SNOVA_INFO("DOT:  {}:{}", options.trusted_ns->host, options.trusted_ns->port);
    uint16_t prefix = payload_len;
    prefix = native_to_big(prefix);
    std::vector<uint8_t> tcp_packet(payload_len + 2);
    memcpy(&tcp_packet[0], &prefix, 2);
    memcpy(&tcp_packet[2], payload, payload_len);
    auto [wn, wec] = co_await server->trusted_ns->AsyncWrite(
        ::asio::buffer(tcp_packet.data(), tcp_packet.size()));
    if (wec) {
      SNOVA_ERROR("Failed to write with error:{}", wec);
      co_return wec;
    }
  } else if (options.trusted_ns->schema == "doh") {
    // SNOVA_INFO("DOH:{}/{}", options.trusted_ns->host, options.trusted_ns->path);
    std::string base64_dns;
    absl::string_view dns_view((const char*)payload, payload_len);
    absl::WebSafeBase64Escape(dns_view, &base64_dns);
    std::string request =
        fmt::format("GET /{}?dns={} HTTP/1.1\r\n", options.trusted_ns->path, base64_dns);
    request.append("Host: ").append(options.trusted_ns->host).append("\r\n");
    request.append("Accept: application/dns-message\r\n\r\n");
    // SNOVA_INFO("DOH request :{}", request);
    auto [wn, wec] =
        co_await server->trusted_ns->AsyncWrite(::asio::buffer(request.data(), request.size()));
    if (wec) {
      SNOVA_ERROR("Failed to write with error:{}", wec);
      co_return wec;
    }
  } else {
    SNOVA_ERROR("unknown dns schema:{}", options.trusted_ns->schema);
  }
  co_return std::error_code{};
}

static asio::awaitable<std::error_code> handle_dns_query(const DNSOptions& options,
                                                         std::shared_ptr<DNSProxyServer> server,
                                                         ::asio::ip::udp::endpoint&& src_endpoint,
                                                         IOBufPtr&& query_buffer,
                                                         size_t query_len) {
  IOBufPtr buffer = std::move(query_buffer);
  DNSProxyState state;
  state.init_mstime =
      std::chrono::duration_cast<milliseconds>(std::chrono::system_clock::now().time_since_epoch())
          .count();
  state.orig_endpoint = std::move(src_endpoint);
  uint16_t txid = 0;
  memcpy(&txid, buffer->data(), 2);
  std::string parsed_domain;

  int parse_rc = dns_parse_name(buffer->data(), 12, query_len, parsed_domain);
  if (0 != parse_rc) {
    state.disable_default_ns = false;
  } else {
    for (const auto& domain : options.trusted_ns_domains) {
      if (absl::StrContains(parsed_domain, domain)) {
        state.disable_default_ns = true;
        state.disable_trusted_ns = false;
        break;
      }
    }
  }
  state.cancel_ns_timeout = TimeWheel::GetInstance()->Add(
      [txid]() -> asio::awaitable<void> {
        SNOVA_ERROR("[{}]DNS query timeout.", txid);
        g_dns_states.erase(txid);
        co_return;
      },
      g_dns_query_timeout_msecs);

  if (!state.disable_trusted_ns && !state.disable_default_ns) {
    state.orig_payload.resize(query_len);
    memcpy(&state.orig_payload[0], buffer->data(), query_len);
  }
  g_dns_states.emplace(txid, std::move(state));
  if (!state.disable_default_ns) {
    SNOVA_INFO("DNS over default ns:{}/{}", options.default_ns->host, options.default_ns->port);
    auto [wec, wn] = co_await server->default_ns->async_send_to(
        ::asio::buffer(buffer->data(), query_len), options.default_ns_endpoint,
        ::asio::experimental::as_tuple(::asio::use_awaitable));
    if (wec) {
      SNOVA_ERROR("Failed to write dns with error:{}", wec);
      co_return wec;
    }
  } else {
    co_await dns_over_trusted(server, options, buffer->data(), query_len);
  }
  co_return std::error_code{};
}

static asio::awaitable<std::error_code> default_ns_loop(std::shared_ptr<DNSProxyServer> server,
                                                        const DNSOptions& options) {
  auto ex = co_await asio::this_coro::executor;
  IOBufPtr buffer = get_iobuf(kMaxChunkSize);
  while (true) {
    ::asio::ip::udp::endpoint endpoint;
    auto [ec, n] = co_await server->default_ns->async_receive_from(
        ::asio::buffer(buffer->data(), kMaxChunkSize), endpoint,
        ::asio::experimental::as_tuple(::asio::use_awaitable));
    if (!ec) {
      uint16_t txid = 0;
      memcpy(&txid, buffer->data(), 2);
      std::vector<DNSAnswer> answers;
      bool send_response = true;
      int rc = dns_parse_answers(buffer->data(), 0, n, answers);
      if (0 == rc) {
        bool matched_iprange = false;
        for (auto& ans : answers) {
          if (ans.IsV4() && options.MatchIPRanges(ans.v4_ip.to_uint())) {
            matched_iprange = true;
            break;
          }
        }

        if (!matched_iprange) {
          auto found = g_dns_states.find(txid);
          if (found != g_dns_states.end()) {
            auto& state = found->second;
            if (!state.disable_trusted_ns && !state.orig_payload.empty()) {
              co_await dns_over_trusted(server, options, state.orig_payload.data(),
                                        state.orig_payload.size());
              send_response = false;
              continue;
            }
          }
        }
      } else {
        send_response = true;
      }
      if (send_response) {
        co_await dns_response(server->server, txid, buffer->data(), n);
      } else {
        // SNOVA_INFO("DNS response:{} by default ns", txid);
      }
    } else {
      SNOVA_ERROR("Failed to recv dns proxy server with error:{}", ec);
    }
  }
  co_return std::error_code{};
}

static asio::awaitable<std::error_code> trusted_ns_loop(std::shared_ptr<DNSProxyServer> server,
                                                        const DNSOptions& options) {
  auto ex = co_await asio::this_coro::executor;
  ::asio::steady_timer timer(ex);
  std::chrono::milliseconds period(500);
  IOBufPtr buffer = get_iobuf(kMaxChunkSize);
  size_t buffer_pos = 0;
  while (true) {
    if (!server->trusted_ns) {
      auto new_tls = std::make_shared<TlsSocket>(ex);
      auto ec = co_await new_tls->AsyncConnect(options.trusted_ns_tcp_endpoint);
      if (ec) {
        SNOVA_ERROR("Failed to connect trusted ns with error:{}", ec);
      } else {
        // SNOVA_INFO("Connect trusted ns success.");
        server->trusted_ns = new_tls;
        buffer_pos = 0;
      }
      if (!server->trusted_ns) {
        timer.expires_after(period);
        co_await timer.async_wait(::asio::experimental::as_tuple(::asio::use_awaitable));
        continue;
      }
    }
    auto [rn, rec] = co_await server->trusted_ns->AsyncRead(
        ::asio::buffer(buffer->data() + buffer_pos, buffer->size() - buffer_pos));
    if (rec || 0 == rn) {
      // SNOVA_ERROR("Failed to recv dns with error:{}/{}", rec, rn);
      server->trusted_ns = nullptr;
      continue;
    }
    buffer_pos += rn;
    int dns_res_rest = 0;
    uint32_t answer_offset = 0;
    uint16_t answer_len = 0;
    uint32_t answer_chunk_len = 0;
    if (server->is_trusted_ns_https) {
      size_t readed_len = 0;
      std::string_view head_end("\r\n\r\n", 4);
      absl::string_view dns_res_view((const char*)(buffer->data()), buffer_pos);
      auto hend_pos = dns_res_view.find(head_end);
      if (hend_pos == std::string_view::npos) {
        continue;
      }
      absl::string_view dns_res_headers_view((const char*)(buffer->data()), hend_pos);
      absl::string_view ok_prefix("HTTP/1.1 200 OK");
      auto pos = dns_res_headers_view.find(ok_prefix);
      if (pos == std::string_view::npos) {
        SNOVA_ERROR("Invalid DOH response:{}", dns_res_headers_view);
        server->trusted_ns = nullptr;
        continue;
      }
      absl::string_view content_length;
      int rc = http_get_header(dns_res_headers_view, "Content-Length:", &content_length);
      if (0 != rc) {
        SNOVA_ERROR("Invalid DOH response:{}", dns_res_headers_view);
        server->trusted_ns = nullptr;
        continue;
      }
      uint32_t content_len;
      if (!absl::SimpleAtoi(content_length, &content_len)) {
        SNOVA_ERROR("Invalid DOH response:{}", dns_res_headers_view);
        server->trusted_ns = nullptr;
        continue;
      }
      int expect_len = hend_pos + 4 + content_len;
      dns_res_rest = expect_len - buffer_pos;
      answer_len = content_len;
      answer_chunk_len = answer_len + hend_pos + 4;
      answer_offset = hend_pos + 4;
    } else {
      if (buffer_pos < 12) {
        continue;
      }
      memcpy(&answer_len, buffer->data(), 2);
      answer_len = big_to_native(answer_len);
      answer_chunk_len = answer_len + 2;
      uint16_t txid = 0;
      memcpy(&txid, buffer->data() + 2, 2);
      dns_res_rest = static_cast<int>(answer_len) + 2 - buffer_pos;
      answer_offset = 2;
    }
    if (dns_res_rest > 0) {
      auto ec = co_await read_exact(*server->trusted_ns,
                                    ::asio::buffer(buffer->data() + buffer_pos, dns_res_rest));
      if (ec) {
        SNOVA_ERROR("Failed to recv rest dns with error:{}", ec);
        server->trusted_ns = nullptr;
        continue;
      }
      buffer_pos += dns_res_rest;
    }
    uint16_t txid = 0;
    memcpy(&txid, buffer->data() + answer_offset, 2);
    SNOVA_INFO("DNS response:{} by trusted https ns", txid);
    co_await dns_response(server->server, txid, buffer->data() + answer_offset, answer_len);
    if (buffer_pos > answer_chunk_len) {
      memmove(buffer->data(), buffer->data() + answer_chunk_len, buffer_pos - answer_chunk_len);
      buffer_pos -= answer_chunk_len;
    } else {
      buffer_pos = 0;
    }
  }
  co_return std::error_code{};
}

static asio::awaitable<std::error_code> server_loop(std::shared_ptr<DNSProxyServer> server,
                                                    const DNSOptions& options) {
  auto ex = co_await asio::this_coro::executor;
  while (true) {
    ::asio::ip::udp::endpoint endpoint;
    IOBufPtr buffer = get_iobuf(kMaxChunkSize);
    auto [ec, n] = co_await server->server->async_receive_from(
        ::asio::buffer(buffer->data(), kMaxChunkSize), endpoint,
        ::asio::experimental::as_tuple(::asio::use_awaitable));
    if (!ec) {
      ::asio::co_spawn(ex,
                       handle_dns_query(options, server, std::move(endpoint), std::move(buffer), n),
                       ::asio::detached);
    } else {
      SNOVA_ERROR("Failed to recv dns proxy server with error:{}", ec);
    }
  }
  co_return std::error_code{};
}

asio::awaitable<std::error_code> start_dns_proxy_server(const NetAddress& addr,
                                                        DNSOptions& options) {
  auto ex = co_await asio::this_coro::executor;
  auto ec = co_await options.Init();
  if (ec) {
    SNOVA_ERROR("Failed to init dns options  with error:{}", ec);
    co_return ec;
  }
  ::asio::ip::udp::endpoint endpoint;
  auto resolve_ec = co_await addr.GetEndpoint(&endpoint);
  if (resolve_ec) {
    co_return resolve_ec;
  }
  UDPSocketPtr udp_socket = std::make_shared<UDPSocket>(ex);
  // ::asio::ip::udp::socket udp_socket(ex);
  udp_socket->open(endpoint.protocol());
  udp_socket->bind(endpoint, ec);
  if (ec) {
    SNOVA_ERROR("Failed to bind {} with error:{}", addr.String(), ec);
    co_return ec;
  }
  UDPSocketPtr default_ns_socket = std::make_shared<UDPSocket>(ex);
  default_ns_socket->open(endpoint.protocol());

  std::shared_ptr<DNSProxyServer> server = std::make_shared<DNSProxyServer>();
  server->default_ns = default_ns_socket;
  server->server = udp_socket;
  server->trusted_ns = std::make_shared<TlsSocket>(ex);
  server->is_trusted_ns_https = (options.trusted_ns->schema == "doh");
  ec = co_await server->trusted_ns->AsyncConnect(options.trusted_ns_tcp_endpoint);
  if (ec) {
    SNOVA_ERROR("Failed to connect trusted ns with error:{}", ec);
    co_return ec;
  }
  ::asio::co_spawn(ex, server_loop(server, options), ::asio::detached);
  ::asio::co_spawn(ex, default_ns_loop(server, options), ::asio::detached);
  ::asio::co_spawn(ex, trusted_ns_loop(server, options), ::asio::detached);
  co_return std::error_code{};
}
}  // namespace snova
