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
#include <stdio.h>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "CLI/App.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"

#include "snova/log/log_macros.h"
#include "snova/mux/mux_client.h"
#include "snova/server/dns_proxy_server.h"
#include "snova/server/entry_server.h"
#include "snova/server/mux_server.h"
#include "snova/server/tunnel_server.h"
#include "snova/util/address.h"
#include "snova/util/dns_options.h"
#include "snova/util/flags.h"
#include "snova/util/misc_helper.h"
#include "snova/util/stat.h"
#include "snova/util/time_wheel.h"

#include "spdlog/fmt/fmt.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#ifndef SNOVA_VERSION
#define SNOVA_VERSION unknown
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
  snova::MuxConnManager::GetInstance()->RegisterStat();
}

int main(int argc, char** argv) {
  CLI::App app{"SNOVA:A private proxy tool for low-end boxes."};
  app.set_version_flag("--version", TOSTRING(SNOVA_VERSION));
  app.set_config("--config", "",
                 "Config file path, all cli options can be set into a toml file as the config.",
                 false);

  std::string log_file;
  app.add_option("--log_file", log_file, "Log file, default is stdout");
  int64_t max_log_file_size = 1024 * 1024;
  app.add_option("--max_log_file_size", max_log_file_size, "Max log file size");
  int32_t max_log_file_num = 2;
  app.add_option("--max_log_file_num", max_log_file_num, "Max log file number");
  bool alsologtostderr = false;
  app.add_option("--alsologtostderr", alsologtostderr, "Also log to stderr");

  std::vector<std::string> multi_listens;
  app.add_option("--listen", multi_listens, "Listen address");

  std::string auth_user = "demo_user";
  app.add_option("--user", auth_user, "Auth user name");
  std::string proxy_server;
  app.add_option("--proxy", proxy_server, "The proxy server to connect remote mux server.");
  std::string remote_server;
  app.add_option("--remote", remote_server, "Remote server address");
  app.add_option("--conn_num_per_server", snova::g_conn_num_per_server,
                 "Remote server connection number per server.");
  app.add_option("--conn_expire_secs", snova::g_connection_expire_secs,
                 "Remote server connection expire seconds, default 1800s.");
  app.add_option("--conn_max_inactive_secs", snova::g_connection_max_inactive_secs,
                 "Close connection if it's inactive 'conn_max_inactive_secs' ago.");
  app.add_option("--max_iobuf_pool_size", snova::g_iobuf_max_pool_size, "IOBuf pool max size");
  app.add_option("--stream_io_timeout_secs", snova::g_stream_io_timeout_secs,
                 "Proxy stream IO timeout secs, default 300s");
  uint32_t stat_log_period_secs = 60;
  app.add_option("--stat_log_period_secs", stat_log_period_secs,
                 "Print stat log every 'stat_log_period_secs', set it to 0 to disable stat log.");

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

  app.add_option("--entry_socket_send_buffer_size", snova::g_entry_socket_send_buffer_size,
                 "Entry server socket send buffer size.");
  app.add_option("--entry_socket_recv_buffer_size", snova::g_entry_socket_recv_buffer_size,
                 "Entry server socket recv buffer size.");

  std::vector<std::string> local_tunnel_opts, remote_tunnel_opts;
  app.add_option("-L", local_tunnel_opts,
                 "Local tunnel options, foramt  <local port>:<remote host>:<remote port>, only "
                 "works with entry node.");
  app.add_option("-R", remote_tunnel_opts,
                 "Remote tunnel options, foramt  <remote port>:<local host>:<local port>, only "
                 "works with exit node.");

  snova::DNSOptions dns_options;
  auto dns_group = app.add_option_group("dns", "dns proxy options.");
  std::string ip_range_file;
  dns_group->add_option("--ip_range_file", ip_range_file, "IP range file for dns proxy");
  std::string default_ns, trusted_ns;
  dns_group->add_option("--default_ns", default_ns, "Default nameserver address");
  dns_group->add_option("--trusted_ns", trusted_ns, "TRusted nameserver address");
  dns_group->add_option("--trusted_ns_domains", dns_options.trusted_ns_domains,
                        "Trusted nameserver domains.");
  dns_group->add_option("--dns_proxy_timeout", snova::g_dns_query_timeout_msecs,
                        "DNS proxy timeout(mills), default 800ms.");

  CLI11_PARSE(app, argc, argv);

  if (!log_file.empty()) {
    auto rotate_file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_file, max_log_file_size, max_log_file_num);
    // Create logger, it use rotate_file_sink.
    std::vector<spdlog::sink_ptr> sinks;
    sinks.emplace_back(std::move(rotate_file_sink));
    if (alsologtostderr) {
      auto console_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
      sinks.emplace_back(std::move(console_sink));
    }
    auto root_logger = std::make_shared<spdlog::logger>("main", sinks.begin(), sinks.end());
    root_logger->set_level(spdlog::level::info);
    root_logger->flush_on(spdlog::level::info);
    spdlog::register_logger(root_logger);
    // Set default logger.
    spdlog::set_default_logger(root_logger);
  }

  if (!default_ns.empty()) {
    auto result = snova::NetAddress::Parse(default_ns);
    if (result.second) {
      error_exit(
          fmt::format("Failed to parse dns address:{} with error:{}", default_ns, result.second));
    } else {
      dns_options.default_ns = std::move(result.first);
      if (0 == dns_options.default_ns->port) {
        dns_options.default_ns->port = 53;
      }
      SNOVA_INFO("DNS default address:{}", default_ns);
    }
  }
  if (!trusted_ns.empty()) {
    auto result = snova::NetAddress::Parse(trusted_ns);
    if (result.second) {
      error_exit(
          fmt::format("Failed to parse dns address:{} with error:{}", trusted_ns, result.second));
    } else {
      dns_options.trusted_ns = std::move(result.first);
      if (0 == dns_options.trusted_ns->port) {
        dns_options.trusted_ns->port = 53;
      }
      SNOVA_INFO("DNS truested address:{}", trusted_ns);
    }
  }
  if (!ip_range_file.empty()) {
    if (!dns_options.LoadIPRangeFromFile(ip_range_file)) {
      error_exit("Failed to load ip range file");
    }
    SNOVA_INFO("Load {} ip ranges from {}", dns_options.ip_range_map.size(), ip_range_file);
  }

  if (!proxy_server.empty()) {
    if (!absl::StartsWith(proxy_server, "http://")) {
      error_exit("Only HTTP proxy supported for '--proxy' now.");
      return -1;
    }
    std::string host_port = proxy_server.substr(7);
    std::vector<absl::string_view> host_ports = absl::StrSplit(host_port, ':');
    if (host_ports.size() != 2) {
      error_exit("Invalid '--proxy' args.");
      return -1;
    }
    try {
      snova::g_http_proxy_port = std::stoi(std::string(host_ports[1]));
    } catch (...) {
      error_exit("Invalid '--proxy' args.");
      return -1;
    }
    snova::GlobalFlags::GetIntance()->SetHttpProxyHost(std::string(host_ports[0]));
  }

  if (!snova::g_is_entry_node && !snova::g_is_middle_node && !snova::g_is_exit_node) {
    error_exit("Need to run as entry/middle/exit.");
    return -1;
  }
  if (((uint8_t)snova::g_is_entry_node + (uint8_t)snova::g_is_middle_node +
       (uint8_t)snova::g_is_exit_node) > 1) {
    error_exit("Only one role coulde be select from entry/middle/exit.");
    return -1;
  }

  if (snova::g_is_middle_node) {
    if (multi_listens.empty()) {
      error_exit("No 'listen' specified for middle node.");
      return -1;
    }
  }
  if (snova::g_is_entry_node) {
    if (multi_listens.empty() && local_tunnel_opts.empty()) {
      error_exit("No 'listen' specified for entry node.");
      return -1;
    }
  }

  if (!local_tunnel_opts.empty() && !snova::g_is_entry_node) {
    error_exit("'-L' tunnel options only works with entry mode.");
    return -1;
  }
  if (!remote_tunnel_opts.empty() && !snova::g_is_exit_node) {
    error_exit("'-L' tunnel options only works with exit mode.");
    return -1;
  }
  for (const auto& addr : local_tunnel_opts) {
    snova::LocalTunnelOption opt;
    if (!snova::LocalTunnelOption::Parse(addr, &opt)) {
      error_exit("Invalid '-L' args.");
      return -1;
    }
    snova::GlobalFlags::GetIntance()->AddLocalTunnelOption(opt);
  }
  for (const auto& addr : remote_tunnel_opts) {
    snova::RemoteTunnelOption opt;
    if (!snova::RemoteTunnelOption::Parse(addr, &opt)) {
      error_exit("Invalid '-R' args.");
      return -1;
    }
    snova::GlobalFlags::GetIntance()->AddRemoteTunnelOption(opt);
  }

  snova::GlobalFlags::GetIntance()->SetRemoteServer(remote_server);
  snova::GlobalFlags::GetIntance()->SetUser(auth_user);
  SNOVA_INFO("Snova start to run as {} node.",
             (snova::g_is_entry_node ? "ENTRY" : (snova::g_is_exit_node ? "EXIT" : "MIDDLE")));
  ::asio::io_context ctx(1);

  if (!remote_server.empty()) {
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
        },
        ::asio::detached);
  }
  std::vector<std::unique_ptr<snova::NetAddress>> listen_addrs;
  uint32_t dns_server_count = 0;
  for (const auto& v : multi_listens) {
    auto result = snova::NetAddress::Parse(v);
    if (result.second) {
      error_exit(fmt::format("Failed to parse listen address:{} with error:{}", v, result.second));
    } else {
      if (result.first->schema == "dns") {
        if (dns_server_count > 0) {
          error_exit("Only ONE dns proxy server alllowed.");
        }
        if (ip_range_file.empty()) {
          error_exit("Missing '--ip_range_file' for dns proxy server.");
        }
        if (!dns_options.trusted_ns || !dns_options.default_ns) {
          error_exit("Missing '--default_ns' or '--trusted_ns' for dns proxy server.");
        }
        dns_server_count++;
      }
      listen_addrs.emplace_back(std::move(result.first));
      SNOVA_INFO("Listen address:{}", v);
    }
  }
  for (auto& listen_addr : listen_addrs) {
    bool is_mux_server = true;  // mux server or entry server
    if (snova::g_is_exit_node || snova::g_is_middle_node) {
      is_mux_server = true;  // all listen in middle/exit node is mux server
    } else {
      if (listen_addr->schema.empty() || listen_addr->schema == "socks5") {
        is_mux_server = false;
      }
    }
    bool is_dns_server = listen_addr->schema == "dns";
    if (is_dns_server) {
      is_mux_server = false;
    }
    if (is_mux_server) {
      ::asio::co_spawn(
          ctx, snova::start_mux_server(*listen_addr, server_cipher_method, server_cipher_key),
          ::asio::detached);
    } else if (is_dns_server) {
      if (listen_addr->port == 0) {
        listen_addr->port = 53;
      }
      ::asio::co_spawn(ctx, snova::start_dns_proxy_server(*listen_addr, dns_options),
                       ::asio::detached);
    } else {
      ::asio::co_spawn(ctx, snova::start_entry_server(*listen_addr), ::asio::detached);
    }
  }

  if (!local_tunnel_opts.empty()) {
    ::asio::co_spawn(
        ctx,
        []() -> asio::awaitable<void> {
          for (const auto& tunnel_opt : snova::GlobalFlags::GetIntance()->GetLocalTunnelOptions()) {
            snova::NetAddress src_addr, dst_addr;
            src_addr.host = "0.0.0.0";
            src_addr.port = tunnel_opt.local_port;
            dst_addr.host = tunnel_opt.remote_host;
            dst_addr.port = tunnel_opt.remote_port;
            auto [_, ec] = co_await snova::start_tunnel_server(src_addr, dst_addr);
            if (ec) {
              error_exit("Failed to start tunnel server.");
            }
          }
        },
        ::asio::detached);
  }

  init_stats();
  if (stat_log_period_secs > 0) {
    ::asio::co_spawn(ctx, snova::start_stat_timer(stat_log_period_secs), ::asio::detached);
  }

  ::asio::co_spawn(ctx, snova::TimeWheel::GetInstance()->Run(), ::asio::detached);
  ctx.run();
  return 0;
}
