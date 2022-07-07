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
#include "snova/util/tunnel_opt.h"
#include <tuple>
#include <vector>
#include "absl/strings/numbers.h"
#include "absl/strings/str_split.h"

namespace snova {

static bool parse_tunnel_opt(const std::string& addr,
                             std::tuple<uint16_t, std::string, uint16_t>* opt) {
  std::vector<absl::string_view> parts = absl::StrSplit(addr, ":");
  if (parts.size() != 3) {
    return false;
  }
  uint32_t port0, port2 = 0;
  if (!absl::SimpleAtoi(parts[0], &port0) || !absl::SimpleAtoi(parts[2], &port2)) {
    return false;
  }
  std::string host(parts[1].data(), parts[1].size());
  *opt = std::make_tuple(static_cast<uint16_t>(port0), host, static_cast<uint16_t>(port2));
  return true;
}

bool LocalTunnelOption::Parse(const std::string& addr, LocalTunnelOption* opt) {
  std::tuple<uint16_t, std::string, uint16_t> result;
  if (!parse_tunnel_opt(addr, &result)) {
    return false;
  }
  opt->local_port = std::get<0>(result);
  opt->remote_host = std::get<1>(result);
  opt->remote_port = std::get<2>(result);
  return true;
}

bool RemoteTunnelOption::Parse(const std::string& addr, RemoteTunnelOption* opt) {
  std::tuple<uint16_t, std::string, uint16_t> result;
  if (!parse_tunnel_opt(addr, &result)) {
    return false;
  }
  opt->remote_port = std::get<0>(result);
  opt->local_host = std::get<1>(result);
  opt->local_port = std::get<2>(result);
  return true;
}
}  // namespace snova
