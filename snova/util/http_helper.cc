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
#include "snova/util/http_helper.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_split.h"
#include "snova/util/misc_helper.h"

namespace snova {
int parse_http_hostport(absl::string_view recv_data, absl::string_view* hostport) {
  return http_get_header(recv_data, "Host:", hostport);
}
int http_get_header(absl::string_view recv_data, absl::string_view header, absl::string_view* val) {
  auto pos = recv_data.find(header);
  if (pos == absl::string_view::npos) {
    return -1;
  }
  absl::string_view crlf = "\r\n";
  auto end_pos = recv_data.find(crlf, pos);
  if (end_pos == absl::string_view::npos) {
    return -1;
  }
  size_t n = (end_pos - pos - header.size());

  absl::string_view tmp(recv_data.data() + pos + header.size(), n);

  *val = absl::StripAsciiWhitespace(tmp);
  // printf("#### n:%d %d  %s\n", n, val->size(), tmp.data());
  return 0;
}

void ws_get_accept_secret_key(absl::string_view key, std::string* accept_key) {
  std::string encode_key(key.data(), key.size());
  encode_key.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
  std::string resp_bin_key;
  sha1_sum(encode_key, resp_bin_key);
  std::string resp_text_key;
  absl::Base64Escape(resp_bin_key, accept_key);
}

}  // namespace snova
