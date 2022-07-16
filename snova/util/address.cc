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
#include "snova/util/address.h"
#include <vector>
#include "absl/strings/str_split.h"
#include "snova/log/log_macros.h"
#include "snova/util/net_helper.h"

namespace snova {
PaserAddressResult NetAddress::Parse(const std::string& addr) {
  absl::string_view url_part = addr;
  std::vector<absl::string_view> schema_url_parts = absl::StrSplit(addr, "://");
  absl::string_view schema;
  if (schema_url_parts.size() == 2) {
    url_part = schema_url_parts[1];
    schema = schema_url_parts[0];
  }
  absl::string_view host_port_part = url_part;
  absl::string_view path;
  auto pos = url_part.find('/');
  if (pos != absl::string_view::npos) {
    host_port_part = url_part.substr(0, pos);
    if (url_part.size() > (pos + 1)) {
      path = url_part.substr(pos + 1);
    }
  }
  uint16_t port = 0;
  absl::string_view host_view;
  std::vector<absl::string_view> host_ports = absl::StrSplit(host_port_part, ':');
  if (host_ports.size() != 2) {
    host_view = host_port_part;
  } else {
    host_view = host_ports[0];
    try {
      port = std::stoi(std::string(host_ports[1]));
    } catch (...) {
      // SNOVA_ERROR("{}", host_ports[1]);
      return PaserAddressResult{nullptr, std::make_error_code(std::errc::invalid_argument)};
    }
  }
  if (host_view.empty()) {
    host_view = "0.0.0.0";
  }
  std::unique_ptr<NetAddress> net_addr = std::make_unique<NetAddress>();
  if (!schema.empty()) {
    net_addr->schema.assign(schema.data(), schema.size());
  }
  if (!path.empty()) {
    net_addr->path.assign(path.data(), path.size());
  }
  net_addr->host.assign(host_view.data(), host_view.size());
  if (net_addr->schema == "https" || net_addr->schema == "tls" || net_addr->schema == "doh") {
    if (port == 0) {
      port = 443;
    }
  } else if (net_addr->schema == "http") {
    if (port == 0) {
      port = 80;
    }
  } else if (net_addr->schema == "dns") {
    if (port == 0) {
      port = 53;
    }
  } else if (net_addr->schema == "dot") {
    if (port == 0) {
      port = 853;
    }
  }
  net_addr->port = port;
  return PaserAddressResult{std::move(net_addr), std::error_code{}};
}

std::string NetAddress::String() const {
  std::string s;
  if (!schema.empty()) {
    s.append(schema).append("://");
  }
  s.append(host).append(":").append(std::to_string(port));
  if (!path.empty()) {
    s.append("/").append(path);
  }
  return s;
}

asio::awaitable<std::error_code> NetAddress::GetEndpoint(
    ::asio::ip::tcp::endpoint* endpoint) const {
  return resolve_endpoint(host, port, endpoint);
}
asio::awaitable<std::error_code> NetAddress::GetEndpoint(
    ::asio::ip::udp::endpoint* endpoint) const {
  return resolve_endpoint(host, port, endpoint);
}
}  // namespace snova
