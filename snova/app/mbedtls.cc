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
#include <string>
#include "mbedtls/build_info.h"
#include "mbedtls/cipher.h"
const unsigned char key_bytes[32] = {0x2a};
const unsigned char iv1[12] = {0x00};
const unsigned char add_data1[] = {0x01, 0x02};

/**
 * @brief int mbedtls_cipher_auth_encrypt_ext( mbedtls_cipher_context_t *ctx,
                         const unsigned char *iv, size_t iv_len,
                         const unsigned char *ad, size_t ad_len,
                         const unsigned char *input, size_t ilen,
                         unsigned char *output, size_t output_len,
                         size_t *olen, size_t tag_len );
 */

std::string encrypt(mbedtls_cipher_context_t *ctx, size_t tag_len, const std::string &data) {
  unsigned char buffer[data.size() + 100];
  size_t olen = 0;
  mbedtls_cipher_auth_encrypt_ext(ctx, iv1, sizeof(iv1), nullptr, 0,
                                  (const unsigned char *)data.data(), data.size(), buffer,
                                  data.size() + 100, &olen, tag_len);
  printf("###Encrypt from %d to %d\n", data.size(), olen);
  return std::string((const char *)buffer, olen);
}

std::string decrypt(mbedtls_cipher_context_t *ctx, size_t tag_len, const std::string &data) {
  unsigned char buffer[data.size() - tag_len];
  size_t olen = 0;
  int rc = mbedtls_cipher_auth_decrypt_ext(ctx, iv1, sizeof(iv1), nullptr, 0,
                                           (const unsigned char *)data.data(), data.size(), buffer,
                                           data.size() + 100, &olen, tag_len);
  printf("###Decrypt from %d to %d with rc:%d\n", data.size(), olen, rc);
  return std::string((const char *)buffer, olen);
}

// std::string decrypt(const std::string &data) {}

/*
 * Print out some information.
 *
 * All of this information was present in the command line argument, but his
 * function demonstrates how each piece can be recovered from (ctx, tag_len).
 */
static void aead_info(const mbedtls_cipher_context_t *ctx, size_t tag_len) {
  mbedtls_cipher_type_t type = mbedtls_cipher_get_type(ctx);
  const mbedtls_cipher_info_t *info = mbedtls_cipher_info_from_type(type);
  const char *ciph = mbedtls_cipher_info_get_name(info);
  int key_bits = mbedtls_cipher_get_key_bitlen(ctx);
  mbedtls_cipher_mode_t mode = mbedtls_cipher_get_cipher_mode(ctx);
  int iv_size = mbedtls_cipher_get_iv_size(ctx);
  const char *mode_str =
      mode == MBEDTLS_MODE_GCM ? "GCM" : mode == MBEDTLS_MODE_CHACHAPOLY ? "ChachaPoly" : "???";

  printf("%s, %d, %s, %u, %d\n", ciph, key_bits, mode_str, (unsigned)tag_len, iv_size);
}

void init_ctx(mbedtls_cipher_context_t &ctx, size_t &tag_len, bool enc) {
  mbedtls_cipher_init(&ctx);
  mbedtls_cipher_type_t type = MBEDTLS_CIPHER_CHACHA20_POLY1305;
  tag_len = 16;
  //   mbedtls_cipher_type_t type = MBEDTLS_CIPHER_NONE;
  //   tag_len = 0;
  mbedtls_cipher_setup(&ctx, mbedtls_cipher_info_from_type(type));
  int key_len = mbedtls_cipher_get_key_bitlen(&ctx);
  mbedtls_cipher_setkey(&ctx, key_bytes, key_len, enc ? MBEDTLS_ENCRYPT : MBEDTLS_DECRYPT);
  aead_info(&ctx, tag_len);
}

int main() {
  mbedtls_cipher_context_t ctx1, ctx2;
  size_t tag_len;
  init_ctx(ctx1, tag_len, true);
  init_ctx(ctx2, tag_len, false);
  std::string data = "hello,world!";
  std::string enc = encrypt(&ctx1, tag_len, data);
  std::string enc1 = encrypt(&ctx1, tag_len, data);
  std::string multi = enc + enc1;
  printf("###Enc:%d %s\n", enc.size(), enc.data());
  std::string dec = decrypt(&ctx2, tag_len, enc);
  printf("###Dec:%d %s\n", dec.size(), dec.data());

  dec = decrypt(&ctx2, tag_len, multi);
  printf("###Dec:%d %s\n", dec.size(), dec.data());

  return 0;
}