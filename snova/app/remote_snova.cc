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
#include <string>
#include <vector>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "absl/strings/match.h"
#include "snova/log/log_macros.h"
#include "snova/server/remote_server.h"
ABSL_FLAG(std::string, listen, "127.0.0.1:48101", "Listen address");
ABSL_FLAG(std::string, cipher_method, "chacha20_poly1305", "Cipher method");
ABSL_FLAG(std::string, cipher_key, "default cipher key", "Cipher key");
int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("remote server for private proxy tools");
  absl::FlagsUsageConfig usage_config;
  usage_config.contains_help_flags = [](absl::string_view path) -> bool {
    if (absl::StartsWith(path, "snova/")) {
      return true;
    }
    return false;
  };
  usage_config.version_string = []() -> std::string { return "v0.0.1"; };
  absl::ParseCommandLine(argc, argv);
  ::asio::io_context ctx;
  std::string listen = absl::GetFlag(FLAGS_listen);
  std::string cipher_method = absl::GetFlag(FLAGS_cipher_method);
  std::string cipher_key = absl::GetFlag(FLAGS_cipher_key);
  ::asio::co_spawn(ctx, snova::start_remote_server(listen, cipher_method, cipher_key),
                   ::asio::detached);
  ctx.run();
  return 0;
}
