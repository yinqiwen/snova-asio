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
#include "snova/util/flags.h"
#include <memory>

namespace snova {
bool g_is_middle_node = false;
bool g_is_entry_node = false;
bool g_is_exit_node = false;
bool g_is_redirect_node = false;
// std::string g_remote_server;
// std::string g_http_proxy_host;
uint16_t g_http_proxy_port = 0;
uint32_t g_conn_num_per_server = 5;
uint32_t g_iobuf_max_pool_size = 64;
uint32_t g_stream_io_timeout_secs = 120;
uint32_t g_connection_expire_secs = 1800;
uint32_t g_connection_max_inactive_secs = 300;
uint32_t g_tcp_write_timeout_secs = 10;
uint32_t g_entry_socket_send_buffer_size = 0;
uint32_t g_entry_socket_recv_buffer_size = 0;

std::shared_ptr<GlobalFlags>& GlobalFlags::GetIntance() {
  static std::shared_ptr<GlobalFlags> s = std::make_shared<GlobalFlags>();
  return s;
}

void GlobalFlags::SetHttpProxyHost(const std::string& s) { http_proxy_host_ = s; }
const std::string& GlobalFlags::GetHttpProxyHost() { return http_proxy_host_; }
void GlobalFlags::SetRemoteServer(const std::string& s) { remote_server_ = s; }
const std::string& GlobalFlags::GetRemoteServer() { return remote_server_; }

}  // namespace snova
