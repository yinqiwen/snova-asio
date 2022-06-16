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
#include "snova/mux/mux_stream.h"
#include <atomic>
#include <memory>
#include "absl/container/flat_hash_map.h"
#include "asio/experimental/as_tuple.hpp"
#include "snova/log/log_macros.h"
#include "spdlog/fmt/bundled/ostream.h"
namespace snova {

struct ClientStreamID {
  uint64_t client_id = 0;
  uint32_t sid = 0;
  bool operator==(const ClientStreamID& other) const {
    return client_id == other.client_id && sid == other.sid;
  }
};
struct ClientStreamIDHasher {
  std::size_t operator()(const ClientStreamID& k) const { return k.client_id + k.sid; }
};

using MuxStreamTable = absl::flat_hash_map<ClientStreamID, MuxStreamPtr, ClientStreamIDHasher>;
static MuxStreamTable g_streams;
static uint32_t g_active_stream_size = 0;
static std::atomic<uint32_t> g_client_sid{1};
static std::atomic<uint32_t> g_server_sid{2};

size_t MuxStream::Size() { return g_streams.size(); }
size_t MuxStream::ActiveSize() { return g_active_stream_size; }

uint32_t MuxStream::NextID(bool is_client) {
  if (is_client) {
    return g_client_sid.fetch_add(2);
  }
  return g_server_sid.fetch_add(2);
}
MuxStreamPtr MuxStream::New(EventWriterFactory&& factory, StreamDataChannelExecutor& ex,
                            uint64_t client_id, uint32_t sid) {
  MuxStreamPtr p(new MuxStream(std::move(factory), ex, sid));
  ClientStreamID id{client_id, sid};
  g_streams.emplace(id, p);
  return p;
}
MuxStreamPtr MuxStream::Get(uint64_t client_id, uint32_t sid) {
  ClientStreamID id{client_id, sid};
  auto found = g_streams.find(id);
  if (found == g_streams.end()) {
    return nullptr;
  }
  return found->second;
}
void MuxStream::Remove(uint64_t client_id, uint32_t sid) {
  ClientStreamID id{client_id, sid};
  g_streams.erase(id);
  // SNOVA_INFO("[{}]Remove stream.", sid);
}

MuxStream::MuxStream(EventWriterFactory&& factory, StreamDataChannelExecutor& ex, uint32_t sid)
    : event_writer_factory_(std::move(factory)),
      data_channel_(ex, 1),
      sid_(sid),
      is_remote_closed_(false) {
  event_writer_ = event_writer_factory_();
  g_active_stream_size++;
}
MuxStream::~MuxStream() { g_active_stream_size--; }

asio::awaitable<std::error_code> MuxStream::Open(const std::string& host, uint16_t port,
                                                 bool is_tcp) {
  std::unique_ptr<StreamOpenRequest> open_request = std::make_unique<StreamOpenRequest>();
  open_request->head.sid = sid_;
  open_request->remote_host = host;
  open_request->remote_port = port;
  open_request->is_tcp = is_tcp;
  bool success = co_await WriteEvent(std::move(open_request));
  if (!success) {
    co_return std::make_error_code(std::errc::no_link);
  }
  co_return std::error_code{};
}

asio::awaitable<std::error_code> MuxStream::Offer(IOBufPtr&& buf, size_t len) {
  auto [ec] =
      co_await data_channel_.async_send(std::error_code{}, std::move(buf), len,
                                        ::asio::experimental::as_tuple(::asio::use_awaitable));
  if (ec) {
    SNOVA_ERROR("[{}]Failed to write event channel with error:{}", sid_, ec);
    co_return ec;
  }
  co_return std::error_code{};
}
asio::awaitable<StreamReadResult> MuxStream::Read() {
  auto [ec, data, len] =
      co_await data_channel_.async_receive(::asio::experimental::as_tuple(::asio::use_awaitable));
  if (ec) {
    co_return StreamReadResult{nullptr, 0, ec};
  }
  co_return StreamReadResult{std::move(data), len, std::error_code{}};
}

asio::awaitable<std::error_code> MuxStream::Write(IOBufPtr&& buf, size_t len) {
  auto chunk = std::make_unique<StreamChunk>();
  chunk->head.sid = sid_;
  chunk->chunk = std::move(buf);
  chunk->chunk_len = len;
  bool success = co_await WriteEvent(std::move(chunk));
  if (!success) {
    co_return std::make_error_code(std::errc::no_link);
  }
  co_return std::error_code{};
}
asio::awaitable<std::error_code> MuxStream::Close(bool close_by_remote) {
  if (is_remote_closed_) {
    co_return std::error_code{};
  }
  SNOVA_INFO("[{}]Close from remote peer:{}", sid_, close_by_remote);
  data_channel_.close();
  is_remote_closed_ = true;
  if (close_by_remote) {
    // do nothing
  } else {
    auto close_ev = std::make_unique<StreamCloseRequest>();
    close_ev->head.sid = sid_;
    bool success = co_await WriteEvent(std::move(close_ev));
    if (!success) {
      co_return std::make_error_code(std::errc::no_link);
    }
  }
  co_return std::error_code{};
}
}  // namespace snova
