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
#include "CLI/App.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"
// #include "absl/flags/flag.h"
// #include "absl/flags/parse.h"
// #include "absl/flags/usage.h"
// #include "absl/flags/usage_config.h"
#include "absl/strings/match.h"
#include "snova/log/log_macros.h"
#include "snova/mux/mux_client.h"
#include "snova/server/local_server.h"
#include "snova/server/remote_server.h"
#include "snova/util/flags.h"
#include "snova/util/misc_helper.h"
#include "snova/util/stat.h"
#include "snova/util/time_wheel.h"
#include "spdlog/fmt/bundled/ostream.h"
#include "spdlog/fmt/fmt.h"

#ifndef SNOVA_VERSION
#define SNOVA_VERSION "unknown"
#endif

static int error_exit(const std::string& error) {
  printf("ERROR: %s\n", error.c_str());
#ifdef _WIN32
  printf("Press Any Key To Exit");
  getchar();
#endif
  exit(-1);
  return -1;
}

static void init_stats() {
  snova::register_io_stat();
  snova::register_mux_stat();
}

int main(int argc, char** argv) {
  CLI::App app{"SNOVA:A private proxy tool for low-end boxes."};
  app.set_version_flag("--version", std::string(SNOVA_VERSION));
  app.set_config("--config", "", "Config file path", false);
  std::string listen = "0.0.0.0:48100";
  app.add_option("--listen", listen, "Listen address");

  std::string auth_user = "demo_user";
  app.add_option("--user", auth_user, "Auth user name");
  app.add_option("--remote", snova::g_remote_server, "Remote server address");
  app.add_option("--conn_num_per_server", snova::g_conn_num_per_server,
                 "Remote server connection number per server.");
  app.add_option("--max_iobuf_pool_size", snova::g_iobuf_max_pool_size, "IOBuf pool max size");
  app.add_option("--stream_io_timeout_secs", snova::g_stream_io_timeout_secs,
                 "Proxy stream IO timeout secs, default 300s");

  std::string client_cipher_method = "chacha20_poly1305";
  std::string client_cipher_key = "default cipher key";
  std::string server_cipher_method = "chacha20_poly1305";
  std::string server_cipher_key = "default cipher key";
  app.add_option("--client_cipher_method", client_cipher_method, "Client cipher method");
  app.add_option("--client_cipher_key", client_cipher_key, "Client cipher key");
  app.add_option("--server_cipher_method", server_cipher_method, "Server cipher method");
  app.add_option("--server_cipher_key", server_cipher_key, "Server cipher key");

  app.add_option("--entry", snova::g_is_entry_node, "Run as entry node.");
  app.add_option("--middle", snova::g_is_middle_node, "Run as middle node.");
  app.add_option("--exit", snova::g_is_exit_node, "Run as exit node.");
  app.add_option("--redirect", snova::g_is_redirect_node, "Run as redirect server for entry node.");

  CLI11_PARSE(app, argc, argv);

  if (!snova::g_is_entry_node && !snova::g_is_middle_node && !snova::g_is_exit_node) {
    error_exit("Need to run as entry/middle/exit.");
    return -1;
  }
  if (((uint8_t)snova::g_is_entry_node + (uint8_t)snova::g_is_middle_node +
       (uint8_t)snova::g_is_exit_node) > 1) {
    error_exit("Only one role coulde be select from entry/middle/exit.");
    return -1;
  }

  if (snova::g_is_entry_node || snova::g_is_middle_node) {
    if (snova::g_remote_server.empty()) {
      error_exit("'remote' is empty for entry/middle node.");
      return -1;
    }
  }
  SNOVA_INFO("Snova start to run as {} node.",
             (snova::g_is_entry_node ? "ENTRY" : (snova::g_is_exit_node ? "EXIT" : "MIDDLE")));
  ::asio::io_context ctx;

  if (snova::g_is_middle_node || snova::g_is_entry_node) {
    uint64_t client_id = snova::random_uint64(0, std::numeric_limits<uint64_t>::max());
    snova::MuxClient::GetInstance()->SetClientId(client_id);
    SNOVA_INFO("Generated client_id:{}", client_id);
    ::asio::co_spawn(
        ctx,
        [&]() -> asio::awaitable<void> {
          auto ec = co_await snova::MuxClient::GetInstance()->Init(auth_user, client_cipher_method,
                                                                   client_cipher_key);
          if (ec) {
            error_exit(fmt::format("Failed to init mux client with error:{}", ec));
            co_return;
          }
          auto ex = co_await asio::this_coro::executor;
          if (snova::g_is_entry_node) {
            ::asio::co_spawn(ex, snova::start_local_server(listen), ::asio::detached);
          } else {
            ::asio::co_spawn(
                ctx, snova::start_remote_server(listen, server_cipher_method, server_cipher_key),
                ::asio::detached);
          }
        },
        ::asio::detached);
  }
  if (snova::g_is_exit_node) {
    ::asio::co_spawn(ctx,
                     snova::start_remote_server(listen, server_cipher_method, server_cipher_key),
                     ::asio::detached);
  }
  init_stats();
  ::asio::co_spawn(ctx, snova::start_stat_timer(30), ::asio::detached);
  ::asio::co_spawn(ctx, snova::TimeWheel::GetInstance()->Run(), ::asio::detached);
  ctx.run();
  return 0;
}
