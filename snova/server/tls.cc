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
#include "snova/log/log_macros.h"
#include "snova/server/entry_server.h"
#include "snova/server/relay.h"
#include "snova/util/sni.h"

namespace snova {
asio::awaitable<bool> handle_tls_connection(
    ::asio::ip::tcp::socket&& s, IOBufPtr&& rbuf, const Bytes& readable_data,
    std::unique_ptr<::asio::ip::tcp::endpoint>&& orig_remote_endpoint) {
  // SNOVA_INFO("Handle proxy connection by tls.");
  uint16_t remote_port = 443;
  if (orig_remote_endpoint) {
    remote_port = orig_remote_endpoint->port();
  }
  ::asio::ip::tcp::socket sock(std::move(s));  //  make rvalue sock not release after co_await
  IOBufPtr conn_read_buffer = std::move(rbuf);
  IOBuf& read_buffer = *conn_read_buffer;
  std::string remote_host;
  int rc = parse_sni(readable_data.data(), readable_data.size(), &remote_host);
  if (0 != rc) {
    SNOVA_ERROR("Failed to read sni with rc:{}", rc);
    co_return false;
  }

  SNOVA_INFO("Retrive SNI:{} from tls connection.", remote_host);
  if (remote_host == "courier.push.apple.com") {
    co_return false;
    // remote_host =
    // if (orig_remote_endpoint) {
    //   remote_host = orig_remote_endpoint->address().to_string();
    // }
  }
  RelayContext relay_ctx;
  relay_ctx.remote_host = std::move(remote_host);
  relay_ctx.remote_port = remote_port;
  relay_ctx.is_tcp = true;
  relay_ctx.is_tls = true;
  co_await relay(std::move(sock), readable_data, relay_ctx);
  co_return true;
}
}  // namespace snova
