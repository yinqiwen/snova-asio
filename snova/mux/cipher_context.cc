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
#include "snova/mux/cipher_context.h"
#include "snova/log/log_macros.h"
#include "snova/util/endian.h"

namespace snova {

CipherContext::CipherContext()
    : cipher_type_(MBEDTLS_CIPHER_NONE),
      encrypt_iv_(0),
      decrypt_iv_(0),
      iv_len_(0),
      cipher_tag_len_(0) {
  mbedtls_cipher_init(&encrypt_ctx_);
  mbedtls_cipher_init(&decrypt_ctx_);
}
CipherContext::~CipherContext() {
  mbedtls_cipher_free(&encrypt_ctx_);
  mbedtls_cipher_free(&decrypt_ctx_);
}

std::unique_ptr<CipherContext> CipherContext::New(const std::string& cipher_method,
                                                  const std::string& cipher_key) {
  mbedtls_cipher_type_t cipher_type;
  size_t tag_len = 0;
  if (cipher_method == "chacha20_poly1305") {
    cipher_type = MBEDTLS_CIPHER_CHACHA20_POLY1305;
    tag_len = 16;
  } else if (cipher_method == "none") {
    cipher_type = MBEDTLS_CIPHER_NONE;
    tag_len = 0;
  } else {
    SNOVA_ERROR("Unsupported cipher method:{}", cipher_method);
    return nullptr;
  }
  CipherContext* p = new CipherContext;
  p->cipher_type_ = cipher_type;
  p->cipher_tag_len_ = tag_len;
  if (MBEDTLS_CIPHER_NONE != cipher_type) {
    mbedtls_cipher_setup(&p->encrypt_ctx_, mbedtls_cipher_info_from_type(cipher_type));
    int key_len = mbedtls_cipher_get_key_bitlen(&p->encrypt_ctx_);
    p->cipher_key_ = cipher_key;
    p->cipher_key_.resize(key_len);
    uint64_t default_iv = 102477889876LL;
    p->UpdateNonce(default_iv);
    p->iv_len_ = mbedtls_cipher_get_iv_size(&p->encrypt_ctx_);

    mbedtls_cipher_setkey(&p->encrypt_ctx_, (const unsigned char*)(p->cipher_key_.data()), key_len,
                          MBEDTLS_ENCRYPT);
    mbedtls_cipher_setup(&p->decrypt_ctx_, mbedtls_cipher_info_from_type(cipher_type));
    mbedtls_cipher_setkey(&p->decrypt_ctx_, (const unsigned char*)(p->cipher_key_.data()), key_len,
                          MBEDTLS_DECRYPT);
  }
  p->encode_buffer_.resize(kMaxChunkSize + kEventHeadSize + kReservedBufferSize);
  p->decode_buffer_.resize(kMaxChunkSize + kReservedBufferSize);
  return std::unique_ptr<CipherContext>(p);
}
void CipherContext::UpdateNonce(uint64_t nonce) {
  encrypt_iv_ = nonce;
  decrypt_iv_ = nonce;
}

int CipherContext::Encrypt(std::unique_ptr<MuxEvent>& in, MutableBytes& out) {
  uint32_t max_encrypt_buffer_size = kEventHeadSize + kMaxChunkSize + 2 * cipher_tag_len_;
  if (out.size() < max_encrypt_buffer_size) {
    SNOVA_ERROR("No enought space to encrypt, requires {} bytes, but given {} bytes",
                max_encrypt_buffer_size, out.size());
    return ERR_NEED_MORE_OUTPUT_BUFFER;
  }
  if (MBEDTLS_CIPHER_NONE == cipher_type_) {
    MutableBytes head_buffer(out.data(), kEventHeadSize);
    in->head.Encode(head_buffer);
    MutableBytes body_buffer(out.data() + kEventHeadSize, out.size() - kEventHeadSize);
    int rc = in->Encode(body_buffer);
    if (0 != rc) {
      return rc;
    }
    size_t total_len = (kEventHeadSize + body_buffer.size());
    out.remove_suffix(out.size() - total_len);
    return 0;
  }

  MutableBytes body_buffer(encode_buffer_.data() + kEventHeadSize,
                           encode_buffer_.size() - kEventHeadSize);
  int rc = in->Encode(body_buffer);
  if (0 != rc) {
    return rc;
  }
  in->head.len = body_buffer.size();
  MutableBytes head_buffer(encode_buffer_.data(), kEventHeadSize);
  rc = in->head.Encode(head_buffer);
  if (0 != rc) {
    return rc;
  }
  uint64_t iv = native_to_big(encrypt_iv_);
  std::vector<unsigned char> nonce(iv_len_);
  // unsigned char nonce[iv_len_];
  size_t olen = 0;
  memset(nonce.data(), 0, iv_len_);
  memcpy(nonce.data(), &iv, sizeof(iv));
  rc = mbedtls_cipher_auth_encrypt_ext(&encrypt_ctx_, nonce.data(), iv_len_, nullptr, 0,
                                       (const unsigned char*)head_buffer.data(), head_buffer.size(),
                                       out.data(), out.size(), &olen, cipher_tag_len_);
  if (0 != rc) {
    SNOVA_ERROR("Failed to encrypt header with rc:{}", rc);
    return rc;
  }
  size_t header_len = olen;
  size_t body_len = 0;
  if (body_buffer.size() > 0) {
    rc = mbedtls_cipher_auth_encrypt_ext(&encrypt_ctx_, nonce.data(), iv_len_, nullptr, 0,
                                         (const unsigned char*)body_buffer.data(),
                                         body_buffer.size(), out.data() + header_len,
                                         out.size() - header_len, &olen, cipher_tag_len_);
    if (0 != rc) {
      SNOVA_ERROR("Failed to encrypt event body with rc:{}", rc);
      return rc;
    }
    body_len = olen;
  }
  size_t total_len = (header_len + body_len);

  if (total_len < out.size()) {
    out.remove_suffix(out.size() - total_len);
  }
  encrypt_iv_++;
  return 0;
}
int CipherContext::Decrypt(const Bytes& in, std::unique_ptr<MuxEvent>& out, size_t& decrypt_len) {
  if (in.size() < (kEventHeadSize + cipher_tag_len_)) {
    return ERR_NEED_MORE_INPUT_DATA;
  }
  decrypt_len = 0;
  MuxEventHead head;
  // unsigned char nonce[iv_len_];
  std::vector<unsigned char> nonce(iv_len_);
  if (MBEDTLS_CIPHER_NONE == cipher_type_) {
    head.Decode(in);
  } else {
    uint64_t iv = native_to_big(decrypt_iv_);
    memset(nonce.data(), 0, iv_len_);
    memcpy(nonce.data(), &iv, sizeof(iv));
    size_t olen = 0;
    unsigned char head_buf[kEventHeadSize];
    int rc = mbedtls_cipher_auth_decrypt_ext(
        &decrypt_ctx_, nonce.data(), iv_len_, nullptr, 0, (const unsigned char*)in.data(),
        kEventHeadSize + cipher_tag_len_, head_buf, kEventHeadSize, &olen, cipher_tag_len_);
    if (0 != rc) {
      SNOVA_ERROR("Failed to decrypt header with rc:{}", rc);
      return rc;
    }
    // SNOVA_INFO("Decrypt header len:{}", olen);
    head.Decode(Bytes{head_buf, kEventHeadSize});
  }
  if (head.len > (kMaxChunkSize + 128)) {
    SNOVA_ERROR("Too large event len:{}", head.len);
    return ERR_INVALID_EVENT;
  }

  decrypt_len += (kEventHeadSize + cipher_tag_len_);
  if (in.size() < (kEventHeadSize + head.len + 2 * cipher_tag_len_)) {
    return ERR_NEED_MORE_INPUT_DATA;
  }
  out = MuxEvent::NewEvent(head);
  if (!out) {
    return ERR_INVALID_EVENT;
  }
  if (head.len == 0) {
    decrypt_iv_++;
    return 0;
  }
  if (MBEDTLS_CIPHER_NONE == cipher_type_) {
    memcpy(decode_buffer_.data(), in.data() + kEventHeadSize, head.len);
  } else {
    size_t olen = 0;
    int rc = mbedtls_cipher_auth_decrypt_ext(
        &decrypt_ctx_, nonce.data(), iv_len_, nullptr, 0,
        (const unsigned char*)in.data() + kEventHeadSize + cipher_tag_len_,
        head.len + cipher_tag_len_, decode_buffer_.data(), decode_buffer_.size(), &olen,
        cipher_tag_len_);
    if (0 != rc) {
      SNOVA_ERROR("Failed to decrypt event body with rc:{}, data len:{}", rc, head.len);
      return rc;
    }
    // SNOVA_INFO("Decrypt total len:{}", olen);
  }
  decrypt_len += (head.len + cipher_tag_len_);
  // SNOVA_INFO("Decrypt buffer len:{}, type:{}", head.len, head.type);
  int rc = out->Decode(Bytes{decode_buffer_.data(), head.len});
  if (0 != rc) {
    out = nullptr;
    return rc;
  }
  decrypt_iv_++;
  return 0;
}

}  // namespace snova
