/*
 *Copyright (c) 2022, qiyingwang <qiyingwang@tencent.com>
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
#include <limits>
#include <string>
#include <vector>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/random/random.h"
#include "snova/log/log_macros.h"
#include "snova/mux/mux_client.h"
#include "snova/server/local_server.h"
ABSL_FLAG(std::string, listen, "127.0.0.1:48100", "Listen address");
ABSL_FLAG(std::string, remote, "", "Remote server address");
ABSL_FLAG(std::string, cipher_method, "chacha20_poly1305", "Cipher method");
ABSL_FLAG(std::string, cipher_key, "default cipher key", "Cipher key");
ABSL_FLAG(std::string, user, "demo_user", "Auth user name");

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  std::string listen = absl::GetFlag(FLAGS_listen);
  std::string auth_user = absl::GetFlag(FLAGS_user);
  std::string cipher_method = absl::GetFlag(FLAGS_cipher_method);
  std::string cipher_key = absl::GetFlag(FLAGS_cipher_key);

  absl::BitGen bitgen;
  uint64_t client_id = absl::Uniform<uint64_t>(bitgen, 0, std::numeric_limits<uint64_t>::max());
  snova::MuxClient::GetInstance()->SetClientId(client_id);
  ::asio::io_context ctx;
  ::asio::co_spawn(
      ctx,
      [&]() -> asio::awaitable<void> {
        auto ec =
            co_await snova::MuxClient::GetInstance()->Init(auth_user, cipher_method, cipher_key);
        if (ec) {
          co_return;
        }
        auto ex = co_await asio::this_coro::executor;
        ::asio::co_spawn(ex, snova::start_local_server(listen), ::asio::detached);
      },
      ::asio::detached);
  ctx.run();
  return 0;
}
