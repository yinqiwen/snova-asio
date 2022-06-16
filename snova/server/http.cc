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
#include <string_view>
#include "absl/strings/ascii.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "absl/strings/strip.h"
#include "asio/experimental/as_tuple.hpp"
#include "snova/io/read_until.h"
#include "snova/log/log_macros.h"
#include "snova/server/entry_server.h"
#include "snova/server/relay.h"
#include "snova/util/http_helper.h"

namespace snova {

asio::awaitable<void> handle_http_connection(::asio::ip::tcp::socket&& s, IOBufPtr&& rbuf,
                                             Bytes& readable_data) {
  SNOVA_INFO("Handle proxy connection by http.");
  ::asio::ip::tcp::socket sock(std::move(s));  //  make rvalue sock not release after co_await
  IOBufPtr conn_read_buffer = std::move(rbuf);
  IOBuf& read_buffer = *conn_read_buffer;
  size_t readed_len = readable_data.size();
  std::string_view head_end("\r\n\r\n", 4);
  // int end_pos = 0;
  int end_pos = co_await read_until(s, read_buffer, readed_len, head_end);
  if (end_pos < 0) {
    co_return;
  }
  readable_data = Bytes(read_buffer.data(), readed_len);
  absl::string_view http_headers_view((const char*)(readable_data.data()), end_pos);
  http_headers_view = absl::StripAsciiWhitespace(http_headers_view);
  absl::string_view host_port_view;
  int rc = parse_http_hostport(http_headers_view, host_port_view);
  if (rc != 0) {
    SNOVA_ERROR("Failed to find Host from request:{}", http_headers_view);
    co_return;
  }
  uint16_t remote_port = 0;
  absl::string_view host_view = host_port_view;
  std::vector<absl::string_view> host_ports = absl::StrSplit(host_port_view, ':');
  if (host_ports.size() == 2) {
    host_view = host_ports[0];
    try {
      remote_port = std::stoi(std::string(host_ports[1]));
    } catch (...) {
      SNOVA_ERROR("Invalid port in Host:{}", host_port_view);
      co_return;
    }
  }

  auto pos = http_headers_view.find(' ');
  if (pos == absl::string_view::npos) {
    SNOVA_ERROR("No method found in request:{}", http_headers_view);
    co_return;
  }
  absl::string_view method_view(http_headers_view.data(), pos);

  if (absl::EqualsIgnoreCase(method_view, "CONNECT")) {
    if (remote_port == 0) {
      remote_port = 443;
    }
    absl::string_view conn_ok = "HTTP/1.0 200 Connection established\r\n\r\n";
    co_await ::asio::async_write(sock, ::asio::buffer(conn_ok.data(), conn_ok.size()),
                                 ::asio::experimental::as_tuple(::asio::use_awaitable));
    // tunnel
    co_await relay(std::move(sock), Bytes{}, std::string(host_view.data(), host_view.size()),
                   remote_port, true);
  } else {
    if (remote_port == 0) {
      remote_port = 80;
    }
    co_await relay(std::move(sock), readable_data, std::string(host_view.data(), host_view.size()),
                   remote_port, true);
  }
  co_return;

  // absl::string_view crlf("\r\n", 2);
  // absl::string_view request_header_view;
  // while (true) {
  //   absl::string_view view((const char*)(readable_data.data()), readable_data.size());
  //   auto pos = view.find(head_end);
  //   if (pos == absl::string_view::npos) {
  //     if (readable_data.size() == read_buffer.size()) {
  //       read_buffer.resize(read_buffer.size() * 2);
  //       readable_data = Bytes{read_buffer.data(), view.size()};
  //     }
  //     auto [ec, n] =
  //         co_await sock.async_read_some(::asio::buffer(read_buffer.data() + readable_data.size(),
  //                                                      read_buffer.size() -
  //                                                      readable_data.size()),
  //                                       ::asio::experimental::as_tuple(::asio::use_awaitable));
  //     if (ec) {
  //       SNOVA_ERROR("Read http connection failed:{}", ec);
  //       co_return;
  //     }
  //     if (0 == n) {
  //       co_return;
  //     }
  //     readable_data = Bytes{read_buffer.data(), view.size() + n};
  //     continue;
  //   }
  //   request_header_view = absl::string_view((const char*)(readable_data.data()), pos);
  //   readable_data.remove_prefix(request_header_view.size());
  //   break;
  // }
  // auto crlf_pos = request_header_view.find(crlf);
  // absl::string_view request_line_view(request_header_view.data(), crlf_pos);
  // request_line_view = absl::StripAsciiWhitespace(request_line_view);
  // std::vector<absl::string_view> parts = absl::StrSplit(request_line_view, ' ');
  // absl::string_view method;
  // absl::string_view url;
  // absl::string_view http_version;
  // for (auto& part : parts) {
  //   part = absl::StripAsciiWhitespace(part);
  //   if (part.size() == 0) {
  //     continue;
  //   }
  //   if (method.empty()) {
  //     method = part;
  //   } else if (url.empty()) {
  //     url = part;
  //   } else if (http_version.empty()) {
  //     http_version = part;
  //   }
  // }
  // SNOVA_INFO("1:{} 2:{} 3:{}", method, url, http_version);
  // uint16_t port = 0;
  // if (absl::StartsWith(url, "http://")) {
  //   url = absl::StripPrefix(url, "http://");
  //   port = 80;
  // } else if (absl::StartsWith(url, "https://")) {
  //   url = absl::StripPrefix(url, "https://");
  //   port = 443;
  // }
  // absl::string_view host_port_view;
  // absl::string_view path_view;
  // auto path_pos = url.find('/');
  // if (path_pos != absl::string_view::npos) {
  //   host_port_view = url.substr(0, path_pos);
  //   path_view = url.substr(path_pos);
  // } else {
  //   host_port_view = url;
  // }

  // absl::string_view host_view = host_port_view;
  // std::vector<absl::string_view> host_ports = absl::StrSplit(host_port_view, ':');
  // if (host_ports.size() == 2) {
  //   host_view = host_ports[0];
  //   try {
  //     port = std::stoi(std::string(host_ports[1]));
  //   } catch (...) {
  //     SNOVA_ERROR("Invalid port in host_port:{}", host_port_view);
  //     co_return;
  //   }
  // }
}
}  // namespace snova
