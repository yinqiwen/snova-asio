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

#pragma once
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "asio.hpp"
#include "asio/experimental/as_tuple.hpp"
#include "asio/experimental/channel.hpp"
#include "snova/log/log_macros.h"
#include "snova/mux/cipher_context.h"
#include "snova/mux/mux_stream.h"

namespace snova {
using ServerAuthResult = std::pair<uint64_t, bool>;
using ServerRelayFunc = std::function<asio::awaitable<void>(uint64_t, std::unique_ptr<MuxEvent>&&)>;

class MuxConnection : public std::enable_shared_from_this<MuxConnection> {
 public:
  static size_t Size();
  static size_t ActiveSize();
  MuxConnection(::asio::ip::tcp::socket&& sock, std::unique_ptr<CipherContext>&& cipher_ctx,
                bool is_local);
  asio::awaitable<bool> ClientAuth(const std::string& user, uint64_t client_id);
  asio::awaitable<ServerAuthResult> ServerAuth();
  asio::awaitable<void> ReadEventLoop();

  void SetServerRelayFunc(ServerRelayFunc&& f) { server_relay_ = std::move(f); }
  void SetIdx(uint32_t idx) { idx_ = idx; }
  uint32_t GetIdx() const { return idx_; }

  ~MuxConnection();
  asio::awaitable<bool> Write(std::unique_ptr<MuxEvent>&& write_ev);
  void Close();

 private:
  template <typename T>
  asio::awaitable<bool> WriteEvent(std::unique_ptr<T>&& event) {
    std::unique_ptr<MuxEvent> write_ev = std::move(event);
    bool r = co_await Write(std::move(write_ev));
    co_return r;
  }
  std::shared_ptr<MuxConnection> GetSelf() { return shared_from_this(); }

  MuxStreamPtr GetStream(uint32_t sid);
  asio::awaitable<int> ProcessReadEvent();

  int ReadEventFromBuffer(std::unique_ptr<MuxEvent>& event, Bytes& buffer);
  asio::awaitable<int> ReadEvent(std::unique_ptr<MuxEvent>& event);

  ::asio::ip::tcp::socket socket_;
  std::unique_ptr<CipherContext> cipher_ctx_;

  std::vector<uint8_t> write_buffer_;
  std::vector<uint8_t> read_buffer_;
  Bytes readable_data_;
  uint64_t client_id_;
  uint32_t idx_;
  ServerRelayFunc server_relay_;
  bool is_local_;
  bool is_authed_;
};
using MuxConnectionPtr = std::shared_ptr<MuxConnection>;
}  // namespace snova