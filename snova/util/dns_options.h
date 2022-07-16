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
#include <memory>
#include <string>
#include <vector>
#include "absl/container/btree_map.h"
#include "asio/ip/address_v4_range.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/ip/udp.hpp"
#include "snova/util/address.h"

namespace snova {
struct DNSOptions {
  ::asio::ip::udp::endpoint default_ns_endpoint;
  ::asio::ip::udp::endpoint trusted_ns_endpoint;
  ::asio::ip::tcp::endpoint trusted_ns_tcp_endpoint;
  std::unique_ptr<NetAddress> default_ns;
  std::unique_ptr<NetAddress> trusted_ns;

  std::vector<std::string> trusted_ns_domains;
  // std::vector<::asio::ip::address_v4_range> ip_ranges;
  absl::btree_map<uint32_t, ::asio::ip::address_v4_range> ip_range_map;

  asio::awaitable<std::error_code> Init();

  bool LoadIPRangeFromFile(const std::string& file);

  bool MatchIPRanges(uint32_t ip) const;
};
}  // namespace snova
