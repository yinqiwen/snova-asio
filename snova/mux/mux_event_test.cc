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
#include <gtest/gtest.h>
#include <utility>
#include "pb_decode.h"
#include "pb_encode.h"
#include "snova/log/log_macros.h"
using namespace snova;  // NOLINT

TEST(MuxEvent, nanopb) {
  snova_AuthRequest message = snova_AuthRequest_init_default;
  message.client_id = 121;
  snprintf(message.user, sizeof(message.user), "%s", "jeeeeee");

  unsigned char buf[256];
  pb_ostream_t output = pb_ostream_from_buffer(buf, sizeof(buf));
  if (!pb_encode_delimited(&output, snova_AuthRequest_fields, &message)) {
    printf("Encoding failed: %s\n", PB_GET_ERROR(&output));
    return;
  }
  size_t n = output.bytes_written;

  snova_AuthRequest decode_msg = snova_AuthRequest_init_default;
  pb_istream_t input = pb_istream_from_buffer(buf, n);
  if (!pb_decode_delimited(&input, snova_AuthRequest_fields, &decode_msg)) {
    printf("Decode failed: %s\n", PB_GET_ERROR(&input));
    return;
  }
  printf("%s %llu %d\n", decode_msg.user, decode_msg.client_id, input.bytes_left);
}
