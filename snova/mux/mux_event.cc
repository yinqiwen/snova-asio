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
#include "snova/mux/mux_event.h"

#include "pb_decode.h"
#include "pb_encode.h"

#include "snova/log/log_macros.h"
#include "snova/util/endian.h"

namespace snova {

int MuxEventHead::Encode(MutableBytes& buffer) {
  if (buffer.size() < kEventHeadSize) {
    SNOVA_ERROR("No enoght space to encode head.");
    return -1;
  }
  uint32_t id = native_to_big(this->sid);
  memcpy(buffer.data(), &id, sizeof(id));
  size_t pos = sizeof(id);
  uint16_t length = native_to_big(this->len);
  memcpy(buffer.data() + pos, &length, sizeof(length));
  pos += sizeof(length);
  buffer.data()[pos] = this->type;
  pos++;
  memcpy(buffer.data() + pos, &(this->flags), 1);
  return 0;
}
int MuxEventHead::Decode(const Bytes& buffer) {
  if (buffer.size() < kEventHeadSize) {
    SNOVA_ERROR("No enoght space to decode head.");
    return -1;
  }
  uint32_t sid;
  memcpy(&sid, buffer.data(), sizeof(sid));
  this->sid = big_to_native(sid);
  size_t pos = sizeof(sid);
  uint16_t len;
  memcpy(&len, buffer.data() + pos, sizeof(len));
  this->len = big_to_native(len);
  pos += sizeof(len);
  this->type = buffer.data()[pos];
  pos++;
  memcpy(&(this->flags), buffer.data() + pos, 1);
  return 0;
}

std::unique_ptr<MuxEvent> MuxEvent::NewEvent(MuxEventHead h) {
  std::unique_ptr<MuxEvent> event;
  switch (h.type) {
    case EVENT_AUTH_REQ: {
      event = std::make_unique<AuthRequest>();
      break;
    }
    case EVENT_AUTH_RES: {
      event = std::make_unique<AuthResponse>();
      break;
    }
    case EVENT_STREAM_OPEN: {
      event = std::make_unique<StreamOpenRequest>();
      break;
    }
    case EVENT_STREAM_CLOSE: {
      event = std::make_unique<StreamCloseRequest>();
      break;
    }
    case EVENT_STREAM_CHUNK: {
      event = std::make_unique<StreamChunk>();
      break;
    }
    default: {
      SNOVA_ERROR("Unknown event type:{}", h.type);
      return nullptr;
    }
  }
  event->head = h;
  return event;
}

// #define RETURN_NOT_OK(s)    \
//   do {                      \
//     int _s = (s);           \
//     if (_s != 0) return _s; \
//   } while (0)

// template <typename T>
// static void encode_int(MutableBytes& buffer, size_t pos, T v) {
//   T enc = native_to_big(v);
//   memcpy(buffer.data() + pos, &enc, sizeof(enc));
// }

// template <typename T>
// static int decode_int(const Bytes& buffer, size_t pos, T& v) {
//   if (buffer.size() < (pos + sizeof(T))) {
//     return ERR_TOO_SMALL_EVENT_DECODE_CONTENT;
//   }
//   memcpy(&v, buffer.data() + pos, sizeof(v));
//   v = big_to_native(v);
//   return 0;
// }

// static void encode_string(MutableBytes& buffer, size_t pos, const std::string& str) {
//   uint16_t elen = str.size();
//   encode_int(buffer, pos, elen);
//   memcpy(buffer.data() + pos + 2, str.data(), str.size());
// }

// static int decode_string(const Bytes& buffer, size_t pos, std::string& str) {
//   uint16_t elen = 0;
//   RETURN_NOT_OK(decode_int(buffer, pos, elen));
//   if (elen >= (kMaxChunkSize - 2)) {
//     return ERR_TOO_LARGE_EVENT_DECODE_LENGTH;
//   }
//   if (buffer.size() < (pos + 2 + elen)) {
//     return ERR_TOO_SMALL_EVENT_DECODE_CONTENT;
//   }
//   str.resize(elen);
//   memcpy(&str[0], buffer.data() + pos + 2, elen);
//   return 0;
// }

// static int decode_bool(const Bytes& buffer, size_t pos, bool& v) {
//   if (buffer.size() < (pos + 1)) {
//     return ERR_TOO_SMALL_EVENT_DECODE_CONTENT;
//   }
//   v = (buffer.data()[pos] == 1);
//   return 0;
// }

int AuthRequest::Encode(MutableBytes& buffer) const {
  pb_ostream_t output = pb_ostream_from_buffer(buffer.data(), buffer.size());
  if (!pb_encode_delimited(&output, snova_AuthRequest_fields, &event)) {
    SNOVA_ERROR("Encoding AuthRequest failed:{}", PB_GET_ERROR(&output));
    return ERR_PB_ENCODE;
  }
  size_t total = output.bytes_written;
  buffer.remove_suffix(buffer.size() - total);
  return 0;

  // uint32_t len = user.size();
  // if (len >= (kMaxChunkSize - 2)) {
  //   return ERR_TOO_LARGE_EVENT_ENCODE_CONTENT;
  // }
  // encode_string(buffer, 0, user);
  // encode_int(buffer, user.size() + 2, client_id);
  // size_t pos = user.size() + 2 + sizeof(client_id);
  // memcpy(buffer.data() + pos, &flags, 1);
  // size_t total = (user.size() + 2 + sizeof(client_id) + 1);
  // buffer.remove_suffix(buffer.size() - total);
  return 0;
}
int AuthRequest::Decode(const Bytes& buffer) {
  pb_istream_t input = pb_istream_from_buffer(buffer.data(), buffer.size());
  if (!pb_decode_delimited(&input, snova_AuthRequest_fields, &event)) {
    SNOVA_ERROR("Decode AuthRequest failed:{}", PB_GET_ERROR(&input));
    return ERR_PB_DECODE;
  }
  return 0;

  // RETURN_NOT_OK(decode_string(buffer, 0, user));
  // RETURN_NOT_OK(decode_int(buffer, user.size() + 2, client_id));
  // size_t pos = user.size() + 2 + sizeof(client_id);
  // if (buffer.size() < (pos + 1)) {
  //   return ERR_TOO_SMALL_EVENT_DECODE_CONTENT;
  // }
  // memcpy(&flags, buffer.data() + pos, 1);
  // return 0;
}

int AuthResponse::Encode(MutableBytes& buffer) const {
  pb_ostream_t output = pb_ostream_from_buffer(buffer.data(), buffer.size());
  if (!pb_encode_delimited(&output, snova_AuthResponse_fields, &event)) {
    SNOVA_ERROR("Encoding AuthResponse failed:{}", PB_GET_ERROR(&output));
    return ERR_PB_ENCODE;
  }
  size_t total = output.bytes_written;
  buffer.remove_suffix(buffer.size() - total);
  return 0;
  // encode_int(buffer, 0, iv);
  // buffer.data()[sizeof(iv)] = (success ? 1 : 0);
  // size_t total = (sizeof(iv) + 1);
  // buffer.remove_suffix(buffer.size() - total);
  // return 0;
}
int AuthResponse::Decode(const Bytes& buffer) {
  pb_istream_t input = pb_istream_from_buffer(buffer.data(), buffer.size());
  if (!pb_decode_delimited(&input, snova_AuthResponse_fields, &event)) {
    SNOVA_ERROR("Decode AuthResponse failed:{}", PB_GET_ERROR(&input));
    return ERR_PB_DECODE;
  }
  return 0;
  // RETURN_NOT_OK(decode_int(buffer, 0, iv));
  // RETURN_NOT_OK(decode_bool(buffer, sizeof(iv), success));
  // return 0;
}

int StreamOpenRequest::Encode(MutableBytes& buffer) const {
  pb_ostream_t output = pb_ostream_from_buffer(buffer.data(), buffer.size());
  if (!pb_encode_delimited(&output, snova_StreamOpenRequest_fields, &event)) {
    SNOVA_ERROR("Encoding StreamOpenRequest failed:{}", PB_GET_ERROR(&output));
    return ERR_PB_ENCODE;
  }
  size_t total = output.bytes_written;
  buffer.remove_suffix(buffer.size() - total);
  return 0;

  // encode_string(buffer, 0, remote_host);
  // size_t pos = remote_host.size() + 2;
  // encode_int(buffer, pos, remote_port);
  // pos += sizeof(remote_port);
  // memcpy(buffer.data() + pos, &flags, 1);
  // pos += 1;
  // buffer.remove_suffix(buffer.size() - pos);
  // return 0;
}
int StreamOpenRequest::Decode(const Bytes& buffer) {
  pb_istream_t input = pb_istream_from_buffer(buffer.data(), buffer.size());
  if (!pb_decode_delimited(&input, snova_StreamOpenRequest_fields, &event)) {
    SNOVA_ERROR("Decode StreamOpenRequest failed:{}", PB_GET_ERROR(&input));
    return ERR_PB_DECODE;
  }
  return 0;
  // RETURN_NOT_OK(decode_string(buffer, 0, remote_host));
  // size_t pos = remote_host.size() + 2;
  // RETURN_NOT_OK(decode_int(buffer, pos, remote_port));
  // pos += sizeof(remote_port);
  // if (buffer.size() < (pos + 1)) {
  //   return ERR_TOO_SMALL_EVENT_DECODE_CONTENT;
  // }
  // memcpy(&flags, buffer.data() + pos, 1);
  // // RETURN_NOT_OK(decode_bool(buffer, pos, is_tcp));
  // return 0;
}

int StreamCloseRequest::Encode(MutableBytes& buffer) const {
  buffer.remove_suffix(buffer.size());
  return 0;
}
int StreamCloseRequest::Decode(const Bytes& buffer) { return 0; }

int RetireConnRequest::Encode(MutableBytes& buffer) const {
  buffer.remove_suffix(buffer.size());
  return 0;
}
int RetireConnRequest::Decode(const Bytes& buffer) { return 0; }

int StreamChunk::Encode(MutableBytes& buffer) const {
  if (buffer.size() < chunk_len) {
    SNOVA_INFO("Require {}bytes, but got {} bytes.", chunk_len, buffer.size());
    return ERR_TOO_LARGE_EVENT_ENCODE_CONTENT;
  }
  // encode_int(buffer, 0, chunk_len);
  // size_t pos = sizeof(chunk_len);
  size_t pos = 0;
  if (chunk_len > 0) {
    memcpy(buffer.data() + pos, chunk->data(), chunk_len);
    pos += chunk_len;
  }
  buffer.remove_suffix(buffer.size() - pos);
  return 0;
}
int StreamChunk::Decode(const Bytes& buffer) {
  // RETURN_NOT_OK(decode_int(buffer, 0, chunk_len));
  chunk_len = head.len;
  if (chunk_len > 2 * kMaxChunkSize) {
    return ERR_INVALID_EVENT;
  }
  if (buffer.size() < chunk_len) {
    return ERR_TOO_SMALL_EVENT_DECODE_CONTENT;
  }
  chunk = get_iobuf(chunk_len);
  memcpy(chunk->data(), buffer.data(), chunk_len);
  return 0;
}

}  // namespace snova
