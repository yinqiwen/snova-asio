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
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include "asio/experimental/channel.hpp"
#include "snova/io/io.h"

#define ERR_NEED_MORE_INPUT_DATA -1000
#define ERR_NEED_MORE_OUTPUT_BUFFER -1001
#define ERR_INVALID_EVENT -1002
#define ERR_TOO_LARGE_EVENT_ENCODE_CONTENT -1003
#define ERR_TOO_LARGE_EVENT_DECODE_LENGTH -1004
#define ERR_READ_EOF -1005
#define ERR_TOO_SMALL_EVENT_DECODE_CONTENT -1006

namespace snova {
static constexpr uint16_t kEventHeadSize = 8;

struct MuxFlags {
  unsigned body_no_encrypt : 1;
  unsigned reserved : 7;
  MuxFlags() {
    body_no_encrypt = 0;
    reserved = 0;
  }
};

struct MuxEventHead {
  uint32_t sid = 0;
  uint16_t len = 0;
  uint8_t type = 0;
  MuxFlags flags;
  int Encode(MutableBytes& buffer);
  int Decode(const Bytes& buffer);
};

enum EventType {
  EVENT_AUTH_REQ = 1,
  EVENT_AUTH_RES,
  EVENT_STREAM_OPEN,
  EVENT_STREAM_CLOSE,
  EVENT_STREAM_CHUNK,
  EVENT_RETIRE_CONN_REQ,
  // EVENT_RETIRE_CONN_RES,
};

struct MuxEvent {
  MuxEventHead head;

  static std::unique_ptr<MuxEvent> NewEvent(MuxEventHead h);
  virtual int Encode(MutableBytes& buffer) const = 0;
  virtual int Decode(const Bytes& buffer) = 0;
  virtual ~MuxEvent() {}
};

// using EventChannel =
//     asio::experimental::channel<void(std::error_code, std::unique_ptr<MuxEvent>&&)>;
// using EventChannelPtr = std::shared_ptr<EventChannel>;
// using EventChannelGetter = std::function<EventChannelPtr()>;

using EventWriter = std::function<asio::awaitable<bool>(std::unique_ptr<MuxEvent>&&)>;
using EventWriterFactory = std::function<EventWriter()>;

struct AuthFlags {
  unsigned is_entry : 1;
  unsigned is_exit : 1;
  unsigned is_middle : 1;
  unsigned reserved : 6;
  AuthFlags() {
    is_entry = 1;
    is_exit = 0;
    reserved = 0;
  }
};
struct AuthRequest : public MuxEvent {
  std::string user;
  uint64_t client_id = 0;
  AuthFlags flags;
  AuthRequest() { head.type = EVENT_AUTH_REQ; }
  int Encode(MutableBytes& buffer) const override;
  int Decode(const Bytes& buffer) override;
};
struct AuthResponse : public MuxEvent {
  bool success;
  uint64_t iv = 0;
  AuthResponse() { head.type = EVENT_AUTH_RES; }
  int Encode(MutableBytes& buffer) const override;
  int Decode(const Bytes& buffer) override;
};

struct RetireConnRequest : public MuxEvent {
  RetireConnRequest() { head.type = EVENT_RETIRE_CONN_REQ; }
  int Encode(MutableBytes& buffer) const override;
  int Decode(const Bytes& buffer) override;
};

struct StreamOpenFlags {
  unsigned is_tcp : 1;
  unsigned is_tls : 1;
  unsigned reserved : 6;
  StreamOpenFlags() {
    is_tcp = 1;
    is_tls = 0;
    reserved = 0;
  }
};

struct StreamOpenRequest : public MuxEvent {
  std::string remote_host;
  uint16_t remote_port = 0;
  StreamOpenFlags flags;
  StreamOpenRequest() { head.type = EVENT_STREAM_OPEN; }
  int Encode(MutableBytes& buffer) const override;
  int Decode(const Bytes& buffer) override;
};
struct StreamCloseRequest : public MuxEvent {
  StreamCloseRequest() { head.type = EVENT_STREAM_CLOSE; }
  int Encode(MutableBytes& buffer) const override;
  int Decode(const Bytes& buffer) override;
};

struct StreamChunk : public MuxEvent {
  IOBufPtr chunk;
  uint32_t chunk_len = 0;
  StreamChunk() { head.type = EVENT_STREAM_CHUNK; }
  int Encode(MutableBytes& buffer) const override;
  int Decode(const Bytes& buffer) override;
};

}  // namespace snova
