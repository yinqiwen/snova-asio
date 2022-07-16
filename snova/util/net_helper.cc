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
#include "snova/util/net_helper.h"
#ifdef __linux__
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#include <array>
#include <memory>
#include <vector>

#include "absl/strings/str_split.h"
#include "asio/experimental/as_tuple.hpp"
#include "snova/io/io_util.h"
#include "snova/log/log_macros.h"
#include "snova/util/endian.h"
#include "spdlog/fmt/fmt.h"

namespace snova {

static std::array<::asio::ip::address_v4_range, 3> g_private_networks = {
    ::asio::ip::make_network_v4("10.0.0.0/8").hosts(),
    ::asio::ip::make_network_v4("172.16.0.0/12").hosts(),
    ::asio::ip::make_network_v4("192.168.0.0/16").hosts(),
};

bool is_private_address(const ::asio::ip::address& addr) {
  if (!addr.is_v4()) {
    return false;
  }
  ::asio::ip::address_v4 v4_addr = addr.to_v4();
  if (v4_addr.is_loopback()) {
    return true;
  }
  for (size_t i = 0; i < sizeof(g_private_networks); i++) {
    if (g_private_networks[i].find(addr.to_v4()) != g_private_networks[i].end()) {
      return true;
    }
  }
  return false;
}

#define SO_ORIGINAL_DST 80
int get_orig_dst(int fd, ::asio::ip::tcp::endpoint* endpoint) {
#ifdef __linux__
  struct sockaddr_in6 serverAddrV6;
  struct sockaddr_in serverAddr;
  socklen_t size = sizeof(serverAddr);
  if (getsockopt(fd, SOL_IP, SO_ORIGINAL_DST, &serverAddr, &size) >= 0) {
    *endpoint = ::asio::ip::tcp::endpoint(
        ::asio::ip::make_address_v4(ntohl(serverAddr.sin_addr.s_addr)), ntohs(serverAddr.sin_port));
    return 0;
  }
  size = sizeof(serverAddrV6);
  if (getsockopt(fd, SOL_IPV6, SO_ORIGINAL_DST, &serverAddrV6, &size) >= 0) {
    std::array<uint8_t, 16> v6_ip;
    memcpy(v6_ip.data(), serverAddrV6.sin6_addr.s6_addr, 16);
    *endpoint = ::asio::ip::tcp::endpoint(::asio::ip::make_address_v6(v6_ip),
                                          ntohs(serverAddrV6.sin6_port));
    return 0;
  }
#endif
  return -1;
}

asio::awaitable<SocketPtr> get_connected_socket(const std::string& host, uint16_t port,
                                                bool is_tcp) {
  auto ex = co_await asio::this_coro::executor;
  ::asio::ip::tcp::endpoint select_endpoint;
  auto resolve_ec = co_await resolve_endpoint(host, port, &select_endpoint);
  if (resolve_ec) {
    SNOVA_ERROR("No endpoint found for {}:{} with error:{}", host, port, resolve_ec);
    co_return nullptr;
  }
  SocketPtr socket = std::make_unique<::asio::ip::tcp::socket>(ex);
  auto [connect_ec] = co_await socket->async_connect(
      select_endpoint, ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (connect_ec) {
    SNOVA_ERROR("Connect {} with error:{}", select_endpoint, connect_ec);
    co_return nullptr;
  }
  co_return socket;
}

template <typename T>
static asio::awaitable<std::error_code> do_resolve_endpoint(
    const std::string& host, uint16_t port, ::asio::ip::basic_endpoint<T>* endpoint) {
  auto ex = co_await asio::this_coro::executor;
  asio::ip::basic_resolver<T> r(ex);
  std::string port_str = std::to_string(port);
  auto [ec, results] = co_await r.async_resolve(
      host, port_str, ::asio::experimental::as_tuple(::asio::use_awaitable));

  if (ec || results.size() == 0) {
    SNOVA_ERROR("No endpoint found for {}:{} with error:{}", host, port_str, ec);
    if (ec) {
      co_return ec;
    }
    co_return std::make_error_code(std::errc::bad_address);
  }
  *endpoint = *(results.begin());
  co_return std::error_code{};
}

asio::awaitable<std::error_code> resolve_endpoint(const std::string& host, uint16_t port,
                                                  ::asio::ip::udp::endpoint* endpoint) {
  return do_resolve_endpoint<asio::ip::udp>(host, port, endpoint);
}

asio::awaitable<std::error_code> resolve_endpoint(const std::string& host, uint16_t port,
                                                  ::asio::ip::tcp::endpoint* endpoint) {
  return do_resolve_endpoint<asio::ip::tcp>(host, port, endpoint);
}

asio::awaitable<std::error_code> connect_remote_via_http_proxy(SocketRef socket,
                                                               const std::string& remote_host,
                                                               uint16_t remote_port,
                                                               const std::string& proxy_host,
                                                               uint16_t proxy_port) {
  ::asio::ip::tcp::endpoint proxy_endpoint;
  auto ec = co_await resolve_endpoint(proxy_host, proxy_port, &proxy_endpoint);
  if (ec) {
    co_return ec;
  }
  auto [connect_ec] = co_await socket.async_connect(
      proxy_endpoint, ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (connect_ec) {
    SNOVA_ERROR("Connect {} with error:{}", proxy_host, connect_ec);
    co_return connect_ec;
  }

  std::string conn_req = fmt::format(
      "CONNECT {}:{} HTTP/1.1\r\nHost: {}:{}\r\nConnection: keep-alive\r\nProxy-Connection: "
      "keep-alive\r\n\r\n",
      remote_host, remote_port, remote_host, remote_port);
  auto [wec, wn] =
      co_await ::asio::async_write(socket, ::asio::buffer(conn_req.data(), conn_req.size()),
                                   ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (wec) {
    SNOVA_ERROR("Write CONNECT request failed with error:{}", wec);
    co_return wec;
  }
  std::string_view head_end("\r\n\r\n", 4);
  IOBufPtr read_buffer = get_iobuf(kMaxChunkSize);
  size_t readed_len = 0;
  int end_pos = co_await read_until(socket, *read_buffer, readed_len, head_end);
  if (end_pos < 0) {
    SNOVA_ERROR("Failed to recv CONNECT response.");
    co_return std::make_error_code(std::errc::connection_refused);
  }
  std::string_view response_view((const char*)read_buffer->data(), end_pos);
  if (absl::StartsWith(response_view, "HTTP/1.1 2") ||
      absl::StartsWith(response_view, "HTTP/1.0 2")) {
    co_return std::error_code{};
  }
  SNOVA_ERROR("Recv CONNECT response status:{} from http proxy.", response_view);
  co_return std::make_error_code(std::errc::connection_refused);
}
#define MAX_STR_LEN 256
int dns_parse_name(const uint8_t* payload, int payload_offset, int payload_len, std::string& name) {
  if (payload_offset == 0 || payload_offset >= payload_len) return -1;

  char tmp[MAX_STR_LEN];
  char* dest = tmp;

  const uint8_t* pstart = payload + payload_offset;
  const uint8_t* p = pstart;
  const uint8_t* end = payload + payload_len;
  while (p < end) {
    uint16_t dot_len = *p;
    if ((dot_len & 0xc0) == 0xc0) {
      if (p > pstart) {
        name = std::string(tmp, static_cast<int>(p - pstart - 1));
      }
      p++;
      std::string sub_name;
      int sub_offset = (uint8_t)*p;
      dns_parse_name(payload, sub_offset, payload_len, sub_name);
      name += '.' + sub_name;
      return 0;
    }
    if (((p + dot_len) >= end)) return -1;
    if (dot_len == 0) {
      if (p > pstart) {
        name = std::string(tmp, static_cast<int>(p - pstart - 1));
      }
      return 0;
    }

    // if we get here, dotLen > 0

    // sanity check on max length of temporary buffer
    if ((dest + dot_len + 1) >= (tmp + sizeof(tmp))) {
      return -1;
    }

    if (dest != tmp) {
      *dest++ = '.';
    }
    p++;
    memcpy(dest, p, dot_len);
    p += dot_len;
    dest += dot_len;
  }
  return -1;
}

static int skip_name(const uint8_t* ptr, int remaining) {
  const uint8_t* p = ptr;
  const uint8_t* end = p + remaining;
  while (p < end) {
    int dot_len = *p;
    if ((dot_len & 0xc0) == 0xc0) {
      // printf("skip_name not linear!\n");
    }
    if (dot_len < 0 || dot_len >= remaining) return -1;
    if (dot_len == 0) return static_cast<int>(p - ptr + 1);
    p += (dot_len + 1);
    remaining -= (dot_len + 1);
  }
  return -1;
}
#define DNS_ANS_TYPE_CNAME 5
#define DNS_ANS_TYPE_A 1      // ipv4 address
#define DNS_ANS_TYPE_AAAA 28  // ipv6
#define DNS_ANS_CLASS_IN 1

bool DNSAnswer::IsV4() const { return type == DNS_ANS_TYPE_A; }
bool DNSAnswer::IsV6() const { return type == DNS_ANS_TYPE_AAAA; }

#define READ_U16(p, v)    \
  do {                    \
    memcpy(&v, p, 2);     \
    v = big_to_native(v); \
  } while (0)

static constexpr uint16_t kDNSAnswerHeaderSize = 12;
int dns_parse_answers(const uint8_t* payload, int payload_offset, int payload_len,
                      std::vector<DNSAnswer>& answers) {
  uint16_t txid = 0;
  uint16_t answers_num = 0, question_num = 0;
  READ_U16(payload, txid);
  READ_U16(payload + 4, question_num);
  READ_U16(payload + 6, answers_num);
  // memcpy(&answers_num, payload + 6, 2);
  // answers_num = big_to_native(answers_num);
  // SNOVA_INFO("txid:{},question_num:{}, answers_num:{}", txid, question_num, answers_num);
  int parse_offset = 12;
  int parse_remaining = payload_len - parse_offset;
  while (question_num > 0) {
    int name_len = skip_name(payload + parse_offset, parse_remaining);
    if (name_len <= 0) {
      SNOVA_ERROR("skip_name failed with question_num:{}", question_num);
      return -1;
    }
    parse_remaining -= (name_len + 4);
    parse_offset += (name_len + 4);
    if (parse_remaining < 0) {
      SNOVA_ERROR("parse_remaining:{}", parse_remaining);
      return -1;
    }
    question_num--;
  }

  while (answers_num > 0) {
    DNSAnswer ans;
    READ_U16(payload + parse_offset, ans.nm);
    int name_offset = 0;
    int namr_rc = 0;
    int fields_offset = 0;
    std::string name;
    if ((ans.nm & 0xc000) == 0xc000) {
      name_offset = ans.nm & 0x3fff;
      namr_rc = dns_parse_name(payload, name_offset, payload_len, name);
      // nameLen = dnsReadName(name, nameOffset, payload, payloadLen);
      fields_offset = 0;
    } else {
      name_offset = parse_offset;
      namr_rc = dns_parse_name(payload, name_offset, payload_len, name);
      fields_offset = name.size();
    }
    if (namr_rc < 0) {
      SNOVA_ERROR("parse ans name failed");
      return -1;
    }

    READ_U16(payload + parse_offset + fields_offset + 2, ans.type);
    READ_U16(payload + parse_offset + fields_offset + 4, ans.cls);
    READ_U16(payload + parse_offset + fields_offset + 10, ans.datalen);
    // SNOVA_INFO("parse name:{}, type:{}, cls:{}, datalen:{}", name, ans.type, ans.cls,
    // ans.datalen);
    if ((parse_offset + fields_offset + kDNSAnswerHeaderSize + ans.datalen) > payload_len) {
      SNOVA_ERROR("no enought space to parse");
      return -1;
    }

    switch (ans.type) {
      case DNS_ANS_TYPE_CNAME: {
        // skip cname
        // std::string cname;
        // dns_parse_name(payload, parse_offset + fields_offset + kDNSAnswerHeaderSize, payload_len,
        //                cname);
        // SNOVA_INFO("parse cname:{}", cname);
        break;
      }
      case DNS_ANS_TYPE_A: {
        std::array<uint8_t, 4> v4_ip;
        memcpy(&v4_ip[0], payload + parse_offset + kDNSAnswerHeaderSize + fields_offset,
               v4_ip.size());
        ans.v4_ip = asio::ip::make_address_v4(v4_ip);
        // SNOVA_INFO("ip:{}", ans.v4_ip.to_string().c_str());
        break;
      }
      case DNS_ANS_TYPE_AAAA: {
        std::array<uint8_t, 16> v6_ip;
        memcpy(&v6_ip[0], payload + parse_offset + kDNSAnswerHeaderSize + fields_offset,
               v6_ip.size());
        ans.v6_ip = asio::ip::make_address_v6(v6_ip);
        break;
      }
      default: {
        SNOVA_ERROR("Invalid answer type:{}", ans.type);
        return -1;
      }
    }
    parse_offset += (kDNSAnswerHeaderSize + fields_offset + ans.datalen);
    answers_num--;
    answers.emplace_back(ans);
  }
  return 0;
}

}  // namespace snova
