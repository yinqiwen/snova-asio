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
#include "snova/util/misc_helper.h"
#include <random>
// #include "mbedtls/sha1.h"
#include "openssl/sha.h"

namespace snova {
uint64_t random_uint64(uint64_t min, uint64_t max) {
  std::random_device rd;      // Get a random seed from the OS entropy device, or whatever
  std::mt19937_64 eng(rd());  // Use the 64-bit Mersenne Twister 19937 generator
                              // and seed it with entropy.

  // Define the distribution, by default it goes from 0 to MAX(unsigned long long)
  // or what have you.
  std::uniform_int_distribution<uint64_t> distr(min, max);
  return distr(eng);
}

void sha1_sum(const std::string& src, std::string& dst) {
  // mbedtls_sha1_context ctx;
  // mbedtls_sha1_init(&ctx);
  // mbedtls_sha1_starts(&ctx);
  // mbedtls_sha1_update(&ctx, reinterpret_cast<const unsigned char*>(src.data()), src.size());
  // unsigned char output[20];
  // mbedtls_sha1_finish(&ctx, output);
  // mbedtls_sha1_free(&ctx);
  // dst.append(reinterpret_cast<const char*>(output), 20);

  SHA_CTX ctx;
  uint8_t hash[SHA_DIGEST_LENGTH];

  SHA1_Init(&ctx);
  SHA1_Update(&ctx, src.data(), src.size());
  SHA1_Final(hash, &ctx);
  dst.append(reinterpret_cast<const char*>(hash), SHA_DIGEST_LENGTH);
}
}  // namespace snova
