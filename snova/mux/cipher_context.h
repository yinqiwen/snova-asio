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
#include <memory>
#include <string>
#include <vector>
#include "mbedtls/cipher.h"
#include "snova/io/io.h"
#include "snova/mux/mux_event.h"
namespace snova {
class CipherContext {
 public:
  ~CipherContext();
  static std::unique_ptr<CipherContext> New(const std::string& cipher_method,
                                            const std::string& cipher_key);
  size_t GetTagLength() const { return cipher_tag_len_; }
  void UpdateNonce(uint64_t nonce);
  int Encrypt(std::unique_ptr<MuxEvent>& in, MutableBytes& out);
  int Decrypt(const Bytes& in, std::unique_ptr<MuxEvent>& out, size_t& decrypt_len);

 private:
  CipherContext();
  mbedtls_cipher_type_t cipher_type_;
  mbedtls_cipher_context_t encrypt_ctx_;
  mbedtls_cipher_context_t decrypt_ctx_;
  std::vector<uint8_t> encode_buffer_;
  std::vector<uint8_t> decode_buffer_;
  std::string cipher_key_;
  uint64_t encrypt_iv_;
  uint64_t decrypt_iv_;
  size_t iv_len_;
  size_t cipher_tag_len_;
};

}  // namespace snova