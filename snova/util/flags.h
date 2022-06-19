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
#include <optional>
#include <string>
namespace snova {
extern bool g_is_middle_node;
extern bool g_is_entry_node;
extern bool g_is_exit_node;
extern bool g_is_redirect_node;

// extern std::string g_remote_server;
// extern std::string g_http_proxy_host;
extern uint16_t g_http_proxy_port;
extern uint32_t g_conn_num_per_server;
extern uint32_t g_connection_expire_secs;
extern uint32_t g_iobuf_max_pool_size;
extern uint32_t g_stream_io_timeout_secs;
extern uint32_t g_tcp_write_timeout_secs;

class GlobalFlags {
 public:
  static std::shared_ptr<GlobalFlags>& GetIntance();
  void SetHttpProxyHost(const std::string& s);
  const std::string& GetHttpProxyHost();
  void SetRemoteServer(const std::string& s);
  const std::string& GetRemoteServer();

 private:
  std::string http_proxy_host_;
  std::string remote_server_;
};

}  // namespace snova
