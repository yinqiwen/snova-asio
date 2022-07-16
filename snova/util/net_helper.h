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
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include "asio.hpp"
namespace snova {

bool is_private_address(const ::asio::ip::address& addr);

int get_orig_dst(int fd, ::asio::ip::tcp::endpoint* endpoint);

using SocketPtr = std::unique_ptr<::asio::ip::tcp::socket>;
asio::awaitable<SocketPtr> get_connected_socket(const std::string& host, uint16_t port,
                                                bool is_tcp);

using SocketRef = ::asio::ip::tcp::socket&;
asio::awaitable<std::error_code> connect_remote_via_http_proxy(SocketRef socket,
                                                               const std::string& remote_host,
                                                               uint16_t remote_port,
                                                               const std::string& proxy_host,
                                                               uint16_t proxy_port);
asio::awaitable<std::error_code> resolve_endpoint(const std::string& host, uint16_t port,
                                                  ::asio::ip::tcp::endpoint* endpoint);
asio::awaitable<std::error_code> resolve_endpoint(const std::string& host, uint16_t port,
                                                  ::asio::ip::udp::endpoint* endpoint);

struct DNSAnswer {
  uint16_t nm;
  uint16_t type;
  uint16_t cls;
  uint16_t ttl1;  // if using uint32, compiler will pad struct.
  uint16_t ttl2;
  uint16_t datalen;
  asio::ip::address_v4 v4_ip;
  asio::ip::address_v6 v6_ip;
  bool IsV4() const;
  bool IsV6() const;
};
int dns_parse_name(const uint8_t* payload, int payload_offset, int payload_len, std::string& name);

int dns_parse_answers(const uint8_t* payload, int payload_offset, int payload_len,
                      std::vector<DNSAnswer>& answers);
}  // namespace snova
