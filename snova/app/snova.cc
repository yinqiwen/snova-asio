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
#include <stdio.h>
#include <limits>
#include <random>
#include <string>
#include <vector>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "absl/strings/match.h"
#include "snova/log/log_macros.h"
#include "snova/mux/mux_client.h"
#include "snova/server/local_server.h"
#include "snova/server/remote_server.h"
#include "snova/util/misc_helper.h"
#include "spdlog/fmt/bundled/ostream.h"
ABSL_FLAG(std::string, listen, "127.0.0.1:48100", "Listen address");
ABSL_FLAG(std::string, remote, "", "Remote server address");
ABSL_FLAG(std::string, cipher_method, "chacha20_poly1305", "Cipher method");
ABSL_FLAG(std::string, cipher_key, "default cipher key", "Cipher key");
ABSL_FLAG(std::string, user, "demo_user", "Auth user name");
ABSL_FLAG(bool, entry, false, "Run as entry node.");
ABSL_FLAG(bool, middle, false, "Run as middle node.");
ABSL_FLAG(bool, exit, false, "Run as exit node.");
ABSL_FLAG(bool, redirect, false, "Run as redirect server for entry node.");

static int error_exit(const std::string& error) {
  printf("ERROR: %s\n", error.c_str());
  printf("Press Any Key To Exit");
  getchar();
  exit(-1);
  return -1;
}

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage("private proxy node server");
  absl::FlagsUsageConfig usage_config;
  usage_config.contains_help_flags = [](absl::string_view path) -> bool {
    if (absl::StartsWith(path, "snova/")) {
      return true;
    }
    return false;
  };
  usage_config.version_string = []() -> std::string { return "v0.0.1"; };
  absl::SetFlagsUsageConfig(usage_config);
  absl::ParseCommandLine(argc, argv);
  std::string listen = absl::GetFlag(FLAGS_listen);
  std::string auth_user = absl::GetFlag(FLAGS_user);
  std::string cipher_method = absl::GetFlag(FLAGS_cipher_method);
  std::string cipher_key = absl::GetFlag(FLAGS_cipher_key);
  bool as_entry = absl::GetFlag(FLAGS_entry);
  bool as_middle = absl::GetFlag(FLAGS_middle);
  bool as_exit = absl::GetFlag(FLAGS_exit);
  if (!as_entry && !as_middle && !as_exit) {
    error_exit("Need to run as entry/middle/exit.");
    return -1;
  }
  if (((uint8_t)as_entry + (uint8_t)as_middle + (uint8_t)as_exit) > 1) {
    error_exit("Only one role coulde be select from entry/middle/exit.");
    return -1;
  }

  if (as_entry || as_middle) {
    if (absl::GetFlag(FLAGS_remote).empty()) {
      error_exit("'remote' is empty for entry/middle node.");
      return -1;
    }
  }
  ::asio::io_context ctx;

  if (as_middle || as_entry) {
    uint64_t client_id = snova::random_uint64(0, std::numeric_limits<uint64_t>::max());
    // absl::BitGen bitgen;
    // uint64_t client_id = absl::Uniform<uint64_t>(bitgen, 0,
    // std::numeric_limits<uint64_t>::max());
    snova::MuxClient::GetInstance()->SetClientId(client_id);
    SNOVA_INFO("Generated client_id:{}", client_id);

    ::asio::co_spawn(
        ctx,
        [&]() -> asio::awaitable<void> {
          auto ec =
              co_await snova::MuxClient::GetInstance()->Init(auth_user, cipher_method, cipher_key);
          if (ec) {
            SNOVA_ERROR("Failed to init mux client with error:{}", ec);
            co_return;
          }
          auto ex = co_await asio::this_coro::executor;
          if (as_entry) {
            ::asio::co_spawn(ex, snova::start_local_server(listen), ::asio::detached);
          } else {
            ::asio::co_spawn(ctx, snova::start_remote_server(listen, cipher_method, cipher_key),
                             ::asio::detached);
          }
        },
        ::asio::detached);
  }
  if (as_exit) {
    ::asio::co_spawn(ctx, snova::start_remote_server(listen, cipher_method, cipher_key),
                     ::asio::detached);
  }
  ctx.run();
  return 0;
}
