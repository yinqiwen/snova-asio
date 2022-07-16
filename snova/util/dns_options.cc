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
#include "snova/util/dns_options.h"
#include <fstream>
#include <string>
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"

namespace snova {
asio::awaitable<std::error_code> DNSOptions::Init() {
  auto ec = co_await default_ns->GetEndpoint(&default_ns_endpoint);
  if (ec) {
    co_return ec;
  }
  ec = co_await trusted_ns->GetEndpoint(&trusted_ns_endpoint);
  if (ec) {
    co_return ec;
  }
  ec = co_await trusted_ns->GetEndpoint(&trusted_ns_tcp_endpoint);
  if (ec) {
    co_return ec;
  }
  co_return std::error_code{};
}
bool DNSOptions::LoadIPRangeFromFile(const std::string& file) {
  std::ifstream input(file.c_str());
  if (input.fail()) {
    return false;
  }
  bool success = true;
  std::string line;
  while (std::getline(input, line)) {
    auto line_view = absl::StripAsciiWhitespace(line);
    auto v4_network = ::asio::ip::make_network_v4(line_view);
    ::asio::ip::address_v4_range r = v4_network.hosts();
    auto ret = ip_range_map.emplace(v4_network.network().to_uint(), r);
    if (!ret.second) {
      //   printf("###Failed to emplace:%s\n", v4_network.network().to_string().c_str());
      success = false;
      break;
    }
    // printf("###%s %u  %u %u\n", v4_network.network().to_string().c_str(), r.size(),
    //        r.begin()->to_uint(), r.end()->to_uint());
    // break;
  }
  input.close();
  return success;
}
bool DNSOptions::MatchIPRanges(uint32_t ip) const {
  auto found = ip_range_map.upper_bound(ip);
  if (found == ip_range_map.end()) {
    return false;
  }
  if (found == ip_range_map.begin()) {
    return false;
  }
  found--;
  const ::asio::ip::address_v4_range& range = found->second;
  if (range.find(::asio::ip::make_address_v4(ip)) != range.end()) {
    return true;
  }
  return false;
}
}  // namespace snova
