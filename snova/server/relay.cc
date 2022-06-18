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
#include "snova/server/relay.h"

#include <type_traits>

#include "absl/cleanup/cleanup.h"
#include "asio/experimental/as_tuple.hpp"
#include "asio/experimental/awaitable_operators.hpp"
#include "snova/io/transfer.h"
#include "snova/mux/mux_client.h"
#include "snova/util/flags.h"
#include "snova/util/net_helper.h"
#include "snova/util/time_wheel.h"

using namespace asio::experimental::awaitable_operators;
namespace snova {

using CloseFunc = std::function<asio::awaitable<std::error_code>()>;
template <typename T>
static asio::awaitable<void> do_relay(T& local_stream, const Bytes& readed_data,
                                      const std::string& remote_host, uint16_t remote_port,
                                      bool is_tcp, bool direct) {
  TransferRoutineFunc transfer_routine;
  uint32_t latest_io_time = time(nullptr);
  CancelFunc cancel_transfer_timeout;
  CloseFunc close_local;
  uint32_t stream_id = 0;
  if constexpr (std::is_same_v<T, ::asio::ip::tcp::socket>) {
    close_local = [&]() -> asio::awaitable<std::error_code> {
      local_stream.close();
      co_return std::error_code{};
    };
  } else {
    stream_id = local_stream->GetID();
    close_local = [&]() -> asio::awaitable<std::error_code> {
      co_return co_await local_stream->Close(false);
    };
  }

  if (g_stream_io_timeout_secs > 0) {
    transfer_routine = [&]() { latest_io_time = time(nullptr); };
    cancel_transfer_timeout = TimeWheel::GetInstance()->Add(
        [stream_id, &latest_io_time, close_local]() -> asio::awaitable<void> {
          SNOVA_ERROR("[{}]Close stream since it's not active since {}s ago.", stream_id,
                      time(nullptr) - latest_io_time);
          //   co_await remote_stream->Close(false);
          co_await close_local();
          co_return;
        },
        [&]() -> uint32_t { return latest_io_time; }, g_stream_io_timeout_secs);
  }

  bool direct_relay = (g_is_exit_node || direct);
  // TODO
  if (direct_relay) {
    auto remote_socket = co_await get_connected_socket(remote_host, remote_port, is_tcp);
    if (remote_socket) {
      if (readed_data.size() > 0) {
        co_await ::asio::async_write(*remote_socket,
                                     ::asio::buffer(readed_data.data(), readed_data.size()),
                                     ::asio::experimental::as_tuple(::asio::use_awaitable));
      }
      try {
        co_await(transfer(local_stream, *remote_socket, transfer_routine) &&
                 transfer(*remote_socket, local_stream, transfer_routine));
      } catch (std::exception& ex) {
        SNOVA_ERROR("ex:{}", ex.what());
      }
    }
    if (cancel_transfer_timeout) {
      cancel_transfer_timeout();
    }
    co_return;
  } else {
    auto ex = co_await asio::this_coro::executor;
    EventWriterFactory factory = MuxClient::GetInstance()->GetEventWriterFactory();
    uint64_t client_id = MuxClient::GetInstance()->GetClientId();
    stream_id = MuxStream::NextID(true);
    MuxStreamPtr remote_stream = MuxStream::New(std::move(factory), ex, client_id, stream_id);
    absl::Cleanup auto_remove_remove_stream = [client_id, stream_id] {
      MuxStream::Remove(client_id, stream_id);
    };
    auto ec = co_await remote_stream->Open(remote_host, remote_port, is_tcp);
    if (readed_data.size() > 0) {
      IOBufPtr tmp_buf = get_iobuf(readed_data.size());
      memcpy(tmp_buf->data(), readed_data.data(), readed_data.size());
      co_await remote_stream->Write(std::move(tmp_buf), readed_data.size());
    }

    try {
      co_await(transfer(local_stream, remote_stream, transfer_routine) &&
               transfer(remote_stream, local_stream, transfer_routine));
    } catch (std::exception& ex) {
      SNOVA_ERROR("ex:{}", ex.what());
    }
    if (cancel_transfer_timeout) {
      cancel_transfer_timeout();
    }
    co_await remote_stream->Close(false);
  }
}

asio::awaitable<void> relay(::asio::ip::tcp::socket&& sock, const Bytes& readed_data,
                            const std::string& remote_host, uint16_t remote_port, bool is_tcp) {
  ::asio::ip::tcp::socket socket(std::move(sock));  //  make rvalue sock not release after co_await
  co_await do_relay(socket, readed_data, remote_host, remote_port, is_tcp, false);
}

asio::awaitable<void> relay(StreamPtr local_stream, const Bytes& readed_data,
                            const std::string& remote_host, uint16_t remote_port, bool is_tcp) {
  co_await do_relay(local_stream, readed_data, remote_host, remote_port, is_tcp, false);
  co_await local_stream->Close(false);
}

asio::awaitable<void> relay_direct(::asio::ip::tcp::socket&& sock, const Bytes& readed_data,
                                   const std::string& remote_host, uint16_t remote_port,
                                   bool is_tcp) {
  ::asio::ip::tcp::socket socket(std::move(sock));  //  make rvalue sock not release after co_await
  co_await do_relay(socket, readed_data, remote_host, remote_port, is_tcp, true);
}

asio::awaitable<void> relay_handler(const std::string& auth_user, uint64_t client_id,
                                    std::unique_ptr<MuxEvent>&& event) {
  std::unique_ptr<MuxEvent> mux_event =
      std::move(event);  // this make rvalue event not release after co_await
  auto ex = co_await asio::this_coro::executor;
  StreamOpenRequest* open_request = dynamic_cast<StreamOpenRequest*>(mux_event.get());
  if (nullptr == open_request) {
    SNOVA_ERROR("null request for EVENT_STREAM_OPEN");
    co_return;
  }
  EventWriterFactory factory =
      MuxConnManager::GetInstance()->GetEventWriterFactory(auth_user, client_id);
  uint32_t local_stream_id = open_request->head.sid;
  MuxStreamPtr local_stream = MuxStream::New(std::move(factory), ex, client_id, local_stream_id);
  absl::Cleanup auto_remove_local_stream = [client_id, local_stream_id] {
    MuxStream::Remove(client_id, local_stream_id);
  };
  co_await do_relay(local_stream, {}, open_request->remote_host, open_request->remote_port,
                    open_request->is_tcp, false);
  co_await local_stream->Close(false);
}

}  // namespace snova
