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

#include "snova/io/ws_socket.h"
#include <string>
#include <utility>
#include "absl/strings/escaping.h"

#include "asio/experimental/as_tuple.hpp"

#include "snova/io/buffered_io.h"
#include "snova/io/io_util.h"
#include "snova/log/log_macros.h"
#include "snova/util/endian.h"
#include "snova/util/http_helper.h"
#include "snova/util/misc_helper.h"

namespace snova {
WebSocket::WebSocket(IOConnectionPtr&& io) { io_ = std::make_unique<BufferedIO>(std::move(io)); }
asio::any_io_executor WebSocket::GetExecutor() { return io_->GetExecutor(); }
asio::awaitable<std::error_code> WebSocket::AsyncConnect(const std::string& host) {
  std::string request = "GET /echo HTTP/1.1\r\n";
  request.append("Host: ").append(host).append("\r\n");
  request.append("User-Agent: Go-http-client/1.1\r\n");
  request.append("Connection: Upgrade\r\n");
  std::string ws_sec_key;
  absl::Base64Escape(std::to_string(time(nullptr)), &ws_sec_key);
  request.append("Sec-WebSocket-Key: ").append(ws_sec_key).append("\r\n");
  request.append("Sec-WebSocket-Version: 13\r\n");
  request.append("Upgrade: websocket\r\n\r\n");
  auto [wn, wec] = co_await io_->AsyncWrite(::asio::buffer(request.data(), request.size()));
  if (wec) {
    co_return wec;
  }
  auto recv_buffer = get_iobuf(kMaxChunkSize);
  auto [n, ec] = co_await io_->AsyncRead(::asio::buffer(recv_buffer->data(), kMaxChunkSize));
  if (ec) {
    co_return ec;
  }
  std::string accept_key;
  ws_get_accept_secret_key(ws_sec_key, &accept_key);
  absl::string_view recv_response(reinterpret_cast<const char*>(recv_buffer->data()), n);
  absl::string_view sec_ws_accept;
  int rc = http_get_header(recv_response, "Sec-WebSocket-Accept:", &sec_ws_accept);
  if (0 != rc) {
    SNOVA_ERROR("No 'Sec-WebSocket-Accept' found in {}", recv_response);
    co_return std::make_error_code(std::errc::bad_message);
  }
  // SNOVA_INFO("{} {}", accept_key, sec_ws_accept);
  is_server_ = false;
  co_return std::error_code{};
}
asio::awaitable<std::error_code> WebSocket::AsyncAccept() {
  auto recv_buffer = get_iobuf(kMaxChunkSize);
  auto [n, ec] = co_await io_->AsyncRead(::asio::buffer(recv_buffer->data(), kMaxChunkSize));
  if (ec) {
    co_return ec;
  }

  absl::string_view recv_request(reinterpret_cast<const char*>(recv_buffer->data()), n);
  SNOVA_INFO("Recv {}", recv_request);
  absl::string_view sec_ws_key;
  int rc = http_get_header(recv_request, "Sec-WebSocket-Key:", &sec_ws_key);
  if (0 != rc) {
    SNOVA_ERROR("No 'Sec-WebSocket-Key:' found in request:{}", recv_request);
    co_return std::make_error_code(std::errc::bad_message);
  }
  std::string resp_text_key;
  ws_get_accept_secret_key(sec_ws_key, &resp_text_key);

  std::string resp = "HTTP/1.1 101 Switching Protocols\r\n";
  resp.append("Upgrade: websocket\r\n");
  resp.append("Connection: Upgrade\r\n");
  resp.append("Sec-WebSocket-Accept: ").append(resp_text_key).append("\r\n\r\n");
  auto [wn, wec] = co_await io_->AsyncWrite(::asio::buffer(resp.data(), resp.size()));
  if (wec) {
    co_return wec;
  }
  is_server_ = true;
  co_return std::error_code{};
}
asio::awaitable<IOResult> WebSocket::AsyncWrite(const std::vector<::asio::const_buffer>& buffers) {
  co_return IOResult{0, std::error_code{}};
}
asio::awaitable<IOResult> WebSocket::AsyncWrite(const asio::const_buffer& buffers) {
  uint8_t head_buffer_data[128];
  size_t data_len = buffers.size();
  // SNOVA_INFO("##WS send {} bytes!", data_len);
  size_t offset = 2;
  head_buffer_data[0] = 0x82;  // binary msg
  if (data_len <= 125) {
    head_buffer_data[1] = (data_len & 0x7F);
  } else if (data_len > 65535) {
    head_buffer_data[1] = 127;
    uint64_t frame_len = native_to_big(static_cast<uint64_t>(data_len));
    memcpy(head_buffer_data + 2, &frame_len, 8);
    offset += 8;
  } else {
    head_buffer_data[1] = 126;
    uint16_t frame_len = native_to_big(static_cast<uint16_t>(data_len));
    memcpy(head_buffer_data + 2, &frame_len, 2);
    offset += 2;
  }
  if (!is_server_) {
    head_buffer_data[1] = (0x80 | head_buffer_data[1]);
    uint8_t mask_key[4];  // random value
    memcpy(head_buffer_data + offset, mask_key, 4);
    offset += 4;
    uint8_t* send_data = reinterpret_cast<uint8_t*>(const_cast<void*>(buffers.data()));
    for (size_t i = 0; i < data_len; i++) {
      send_data[i] = (send_data[i] ^ mask_key[i % 4]);
    }
  } else {
    // memcpy(head_buffer_data + offset, buffers.data(), data_len);
  }
  std::vector<::asio::const_buffer> send_bufs;
  send_bufs.push_back(::asio::buffer(head_buffer_data, offset));
  send_bufs.push_back(buffers);
  auto [n, ec] = co_await io_->AsyncWrite(send_bufs);
  co_return IOResult{data_len, ec};
}
// size_t WebSocket::ReadBuffer(const asio::mutable_buffer& buffers) {
//   if (recv_msg_.size() > 0) {
//     size_t len = recv_msg_.size();
//     if (buffers.size() < len) {
//       len = buffers.size();
//     }
//     memcpy(buffers.data(), recv_msg_.data(), len);
//     recv_msg_.remove_prefix(len);
//     return len;
//   }
//   return 0;
// }
asio::awaitable<IOResult> WebSocket::AsyncRead(const asio::mutable_buffer& buffers) {
  uint8_t header_bytes[2];
  auto ec = co_await read_exact(*io_, ::asio::buffer(header_bytes, 2));
  if (ec) {
    co_return IOResult{0, ec};
  }
  uint8_t byte0 = header_bytes[0];
  uint8_t frame_fin = (byte0 & 0x1);
  uint8_t frame_opcode = (byte0 & 0x0F);
  uint8_t byte1 = header_bytes[1];
  uint8_t frame_mask = (byte1 >> 7);
  uint8_t frame_payload_len = (byte1 & 0x7F);
  uint64_t data_msg_len = frame_payload_len;
  if (frame_opcode != 0x2) {
    // onlyu accept binary msg
    // Close();
    // co_return IOResult{0, std::error_code{}};
    SNOVA_ERROR("Recv unexpected {}", frame_opcode);
  }

  // SNOVA_INFO("Recv data opcode:{}, len:{},frame_opcode:{},frame_mask:{},data_msg_len:{} ",
  //            frame_opcode, n, frame_opcode, frame_mask, data_msg_len);
  if (frame_payload_len == 126) {
    uint16_t data_len = 0;
    ec = co_await read_exact(*io_, ::asio::buffer(reinterpret_cast<uint8_t*>(&data_len), 2));
    if (ec) {
      co_return IOResult{0, ec};
    }
    data_len = big_to_native(data_len);
    data_msg_len = data_len;
  } else if (frame_payload_len == 127) {
    uint64_t data_len = 0;
    ec = co_await read_exact(*io_, ::asio::buffer(reinterpret_cast<uint8_t*>(&data_len), 8));
    if (ec) {
      co_return IOResult{0, ec};
    }
    data_len = big_to_native(data_len);
    data_msg_len = data_len;
  }
  if (buffers.size() < data_msg_len) {
    co_return IOResult{0, std::make_error_code(std::errc::no_buffer_space)};
  }
  // SNOVA_INFO("##WS recv {} bytes with mask:{}!", data_msg_len, frame_mask);

  if (frame_mask == 1) {
    uint8_t mask_key[4];
    ec = co_await read_exact(*io_, ::asio::buffer(mask_key, 4));
    if (ec) {
      co_return IOResult{0, ec};
    }
    ec = co_await read_exact(*io_, ::asio::buffer(buffers.data(), data_msg_len));
    if (ec) {
      co_return IOResult{0, ec};
    }
    uint8_t* buffer_bytes = reinterpret_cast<uint8_t*>(buffers.data());
    for (size_t i = 0; i < data_msg_len; i++) {
      buffer_bytes[i] ^= mask_key[i % 4];
    }
  } else {
    ec = co_await read_exact(*io_, ::asio::buffer(buffers.data(), data_msg_len));
    if (ec) {
      co_return IOResult{0, ec};
    }
  }
  co_return IOResult{data_msg_len, std::error_code{}};
}
void WebSocket::Close() { io_->Close(); }
}  // namespace snova
