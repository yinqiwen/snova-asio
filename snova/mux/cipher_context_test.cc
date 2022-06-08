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
#include "snova/mux/cipher_context.h"
#include <gtest/gtest.h>
#include "snova/log/log_macros.h"
using namespace snova;

TEST(CipherContext, Chacha20Poly1305) {
  std::unique_ptr<CipherContext> ctx = CipherContext::New("chacha20_poly1305", "hello,world");
  std::unique_ptr<AuthRequest> auth = std::make_unique<AuthRequest>();
  auth->head.sid = 101;
  std::string user = "test_user";
  auth->user = user;
  std::unique_ptr<MuxEvent> event = std::move(auth);
  std::vector<uint8_t> buffer(8192 * 2);
  MutableBytes mbuffer(buffer.data(), buffer.size());
  int rc = ctx->Encrypt(event, mbuffer);
  EXPECT_EQ(0, rc);
  SNOVA_INFO("Encrypt event size:{}", mbuffer.size());

  Bytes rbuffer(mbuffer.data(), mbuffer.size());
  std::unique_ptr<MuxEvent> decrypt_event;
  size_t decrypt_len;
  rc = ctx->Decrypt(rbuffer, decrypt_event, decrypt_len);
  EXPECT_EQ(0, rc);
  EXPECT_EQ(decrypt_len, mbuffer.size());
  AuthRequest* req = dynamic_cast<AuthRequest*>(decrypt_event.get());
  EXPECT_EQ(req->user, user);
}
