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
#include "snova/server/local_relay.h"

#include "snova/io/transfer.h"
#include "snova/mux/mux_client.h"

#include "asio/experimental/awaitable_operators.hpp"

using namespace asio::experimental::awaitable_operators;
namespace snova {

asio::awaitable<void> client_relay(::asio::ip::tcp::socket&& sock, const Bytes& readed_data,
                                   const std::string& remote_host, uint16_t remote_port,
                                   bool is_tcp) {
  ::asio::ip::tcp::socket socket(std::move(sock));  //  make rvalue sock not release after co_await
  auto ex = co_await asio::this_coro::executor;

  EventWriterFactory factory = MuxClient::GetEventWriterFactory();
  MuxStreamPtr stream = MuxStream::New(std::move(factory), ex, MuxStream::NextID(true));
  auto ec = co_await stream->Open(remote_host, remote_port, is_tcp);
  if (readed_data.size() > 0) {
    IOBufPtr tmp_buf = get_iobuf(readed_data.size());
    memcpy(tmp_buf->data(), readed_data.data(), readed_data.size());
    co_await stream->Write(std::move(tmp_buf), readed_data.size());
  }
  try {
    co_await(transfer(socket, stream) && transfer(stream, socket));
  } catch (std::exception& ex) {
    SNOVA_ERROR("ex:{}", ex.what());
  }
  co_await stream->Close(false);
}

asio::awaitable<void> client_relay(StreamPtr local_stream, const Bytes& readed_data,
                                   const std::string& remote_host, uint16_t remote_port,
                                   bool is_tcp) {
  auto ex = co_await asio::this_coro::executor;

  EventWriterFactory factory = MuxClient::GetEventWriterFactory();
  MuxStreamPtr remote_stream = MuxStream::New(std::move(factory), ex, MuxStream::NextID(true));
  auto ec = co_await remote_stream->Open(remote_host, remote_port, is_tcp);
  if (readed_data.size() > 0) {
    IOBufPtr tmp_buf = get_iobuf(readed_data.size());
    memcpy(tmp_buf->data(), readed_data.data(), readed_data.size());
    co_await remote_stream->Write(std::move(tmp_buf), readed_data.size());
  }
  try {
    co_await(transfer(local_stream, remote_stream) && transfer(remote_stream, local_stream));
  } catch (std::exception& ex) {
    SNOVA_ERROR("ex:{}", ex.what());
  }
  co_await remote_stream->Close(false);
  co_await local_stream->Close(false);
}

}  // namespace snova
