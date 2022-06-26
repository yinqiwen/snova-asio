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
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "asio.hpp"
#include "asio/experimental/as_tuple.hpp"
#include "asio/experimental/channel.hpp"
#include "snova/log/log_macros.h"
#include "snova/mux/cipher_context.h"
#include "snova/mux/mux_stream.h"
#include "snova/util/async_channel_mutex.h"
// #include "snova/util/async_mutex.h"

namespace snova {
using ServerAuthResult = std::tuple<std::string, uint64_t, bool>;
using RelayHandler =
    std::function<asio::awaitable<void>(const std::string&, uint64_t, std::unique_ptr<MuxEvent>&&)>;
class MuxConnection;
using RetireCallback = std::function<void(MuxConnection*)>;
using MuxConnectionPtr = std::shared_ptr<MuxConnection>;
class MuxConnection : public std::enable_shared_from_this<MuxConnection> {
 public:
  static size_t Size();
  static size_t ActiveSize();
  MuxConnection(IOConnectionPtr&& conn, std::unique_ptr<CipherContext>&& cipher_ctx, bool is_local);
  asio::awaitable<bool> ClientAuth(const std::string& user, uint64_t client_id);
  asio::awaitable<ServerAuthResult> ServerAuth();
  asio::awaitable<void> ReadEventLoop();
  MuxConnectionPtr GetShared() { return shared_from_this(); }

  void SetRelayHandler(RelayHandler&& f) { server_relay_ = std::move(f); }
  void SetRetireCallback(RetireCallback&& f) { retire_callback_ = std::move(f); }
  void SetIdx(uint32_t idx) { idx_ = idx; }
  uint32_t GetIdx() const { return idx_; }
  uint32_t GetExpireAtUnixSecs() const { return expire_at_unix_secs_; }
  uint64_t GetClientId() const { return client_id_; }
  uint32_t GetLastActiveUnixSecs() const {
    return last_active_write_unix_secs_ > last_active_read_unix_secs_ ? last_active_write_unix_secs_
                                                                      : last_active_read_unix_secs_;
  }
  uint64_t GetRecvBytes() const { return recv_bytes_; }
  uint64_t GetSendBytes() const { return send_bytes_; }
  bool IsRetired() const { return retired_; }
  void SetRetired() { retired_ = true; }
  uint64_t GetLatestWindowRecvBytes() const;
  uint64_t GetLatestWindowSendBytes() const;
  std::string GetReadState() const;
  void ResetCounter(uint32_t now);

  int ComparePriority(const MuxConnection& other) const;

  ~MuxConnection();
  template <typename T>
  asio::awaitable<bool> WriteEvent(std::unique_ptr<T>&& event) {
    if constexpr (std::is_same_v<T, StreamCloseRequest>) {
      SNOVA_INFO("[{}][{}]Send close to remote.", idx_, event->head.sid);
    }
    std::unique_ptr<MuxEvent> write_ev = std::move(event);
    bool r = co_await Write(std::move(write_ev));
    co_return r;
  }
  asio::awaitable<bool> Write(std::unique_ptr<MuxEvent>&& write_ev);
  void Close();

 private:
  std::shared_ptr<MuxConnection> GetSelf() { return shared_from_this(); }

  MuxStreamPtr GetStream(uint32_t sid);
  asio::awaitable<int> ProcessReadEvent();

  int ReadEventFromBuffer(std::unique_ptr<MuxEvent>& event, Bytes& buffer);
  asio::awaitable<int> ReadEvent(std::unique_ptr<MuxEvent>& event);

  //::asio::ip::tcp::socket socket_;
  IOConnectionPtr io_conn_;
  std::unique_ptr<CipherContext> cipher_ctx_;
  AsyncChannelMutex write_mutex_;
  // cppcoro::async_mutex write_mutex_;

  std::vector<uint8_t> write_buffer_;
  std::vector<uint8_t> read_buffer_;
  Bytes readable_data_;
  std::string auth_user_;
  uint64_t client_id_;
  uint32_t idx_;
  uint32_t expire_at_unix_secs_;
  uint32_t last_unmatch_stream_id_;
  uint32_t last_active_write_unix_secs_;
  uint32_t last_active_read_unix_secs_;
  uint64_t recv_bytes_;
  uint64_t send_bytes_;
  uint32_t read_state_;
  std::vector<uint32_t> latest_window_recv_bytes_;
  std::vector<uint32_t> latest_window_send_bytes_;
  RelayHandler server_relay_;
  RetireCallback retire_callback_;
  bool is_local_;
  bool is_authed_;
  bool retired_;
};

}  // namespace snova
