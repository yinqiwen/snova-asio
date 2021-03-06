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
#include "snova/util/sni.h"
#include <string.h>
#include "snova/log/log_macros.h"
/**
 * Copy&modify from https://github.com/dlundquist/sniproxy/blob/master/src/tls.c
 *
 */
namespace snova {
#define SERVER_NAME_LEN 256
#define TLS_HEADER_LEN 5
#define TLS_HANDSHAKE_CONTENT_TYPE 0x16
#define TLS_HANDSHAKE_TYPE_CLIENT_HELLO 0x01

#ifndef MIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#endif

static int parse_tls_header(const uint8_t *, size_t, std::string *);
static int parse_extensions(const uint8_t *, size_t, std::string *);
static int parse_server_name_extension(const uint8_t *, size_t, std::string *);

static const char tls_alert[] = {
    0x15,       /* TLS Alert */
    0x03, 0x01, /* TLS version  */
    0x00, 0x02, /* Payload length */
    0x02, 0x28, /* Fatal, handshake failure */
};

/* Parse a TLS packet for the Server Name Indication extension in the client
 * hello handshake, returning the first servername found (pointer to static
 * array)
 *
 * Returns:
 *  >=0  - length of the hostname and updates *hostname
 *         caller is responsible for freeing *hostname
 *  -1   - Incomplete request
 *  -2   - No Host header included in this request
 *  -3   - Invalid hostname pointer
 *  -4   - malloc failure
 *  < -4 - Invalid TLS client hello
 */
static int parse_tls_header(const uint8_t *data, size_t data_len, std::string *hostname) {
  uint8_t tls_content_type;
  uint8_t tls_version_major;
  uint8_t tls_version_minor;
  size_t pos = TLS_HEADER_LEN;
  size_t len;

  //   if (hostname == NULL) return -3;

  /* Check that our TCP payload is at least large enough for a TLS header */
  if (data_len < TLS_HEADER_LEN) return -1;

  /* SSL 2.0 compatible Client Hello
   *
   * High bit of first byte (length) and content type is Client Hello
   *
   * See RFC5246 Appendix E.2
   */
  if (data[0] & 0x80 && data[2] == 1) {
    SNOVA_ERROR("Received SSL 2.0 Client Hello which can not support SNI.");
    return -2;
  }

  tls_content_type = data[0];
  if (tls_content_type != TLS_HANDSHAKE_CONTENT_TYPE) {
    SNOVA_ERROR("Request did not begin with TLS handshake.");
    return -5;
  }

  tls_version_major = data[1];
  tls_version_minor = data[2];
  if (tls_version_major < 3) {
    SNOVA_ERROR("Received SSL {}.{} handshake which can not support SNI.", tls_version_major,
                tls_version_minor);
    return -2;
  }

  /* TLS record length */
  len = (static_cast<size_t>(data[3]) << 8) + static_cast<size_t>(data[4]) + TLS_HEADER_LEN;
  data_len = MIN(data_len, len);

  /* Check we received entire TLS record length */
  if (data_len < len) return -1;

  /*
   * Handshake
   */
  if (pos + 1 > data_len) {
    return -5;
  }
  if (data[pos] != TLS_HANDSHAKE_TYPE_CLIENT_HELLO) {
    SNOVA_ERROR("Not a client hello");

    return -5;
  }

  /* Skip past fixed length records:
     1	Handshake Type
     3	Length
     2	Version (again)
     32	Random
     to	Session ID Length
   */
  pos += 38;

  /* Session ID */
  if (pos + 1 > data_len) return -5;
  len = static_cast<size_t>(data[pos]);
  pos += 1 + len;

  /* Cipher Suites */
  if (pos + 2 > data_len) return -5;
  len = (static_cast<size_t>(data[pos]) << 8) + static_cast<size_t>(data[pos + 1]);
  pos += 2 + len;

  /* Compression Methods */
  if (pos + 1 > data_len) return -5;
  len = static_cast<size_t>(data[pos]);
  pos += 1 + len;

  if (pos == data_len && tls_version_major == 3 && tls_version_minor == 0) {
    SNOVA_ERROR("Received SSL 3.0 handshake without extensions");
    return -2;
  }

  /* Extensions */
  if (pos + 2 > data_len) return -5;
  len = (static_cast<size_t>(data[pos] << 8)) + static_cast<size_t>(data[pos + 1]);
  pos += 2;

  if (pos + len > data_len) return -5;
  return parse_extensions(data + pos, len, hostname);
}

static int parse_extensions(const uint8_t *data, size_t data_len, std::string *hostname) {
  size_t pos = 0;
  size_t len;

  /* Parse each 4 bytes for the extension header */
  while (pos + 4 <= data_len) {
    /* Extension Length */
    len = (static_cast<size_t>(data[pos + 2]) << 8) + static_cast<size_t>(data[pos + 3]);

    /* Check if it's a server name extension */
    if (data[pos] == 0x00 && data[pos + 1] == 0x00) {
      /* There can be only one extension of each type, so we break
         our state and move p to beinnging of the extension here */
      if (pos + 4 + len > data_len) return -5;
      return parse_server_name_extension(data + pos + 4, len, hostname);
    }
    pos += 4 + len; /* Advance to the next extension header */
  }
  /* Check we ended where we expected to */
  if (pos != data_len) return -5;

  return -2;
}

static int parse_server_name_extension(const uint8_t *data, size_t data_len,
                                       std::string *hostname) {
  size_t pos = 2; /* skip server name list length */
  size_t len;

  while (pos + 3 < data_len) {
    len = (static_cast<size_t>(data[pos + 1]) << 8) + static_cast<size_t>(data[pos + 2]);

    if (pos + 3 + len > data_len) return -5;

    switch (data[pos]) { /* name type */
      case 0x00:         /* host_name */
        // *hostname = malloc(len + 1);
        // if (*hostname == NULL) {
        //   err("malloc() failure");
        //   return -4;
        // }
        hostname->assign((const char *)(data + pos + 3), len);
        // strncpy(*hostname, (const char *)(data + pos + 3), len);

        // (*hostname)[len] = '\0';

        return 0;
      default: {
        SNOVA_ERROR("Unknown server name extension name type:{}", data[pos]);
        break;
      }
    }
    pos += 3 + len;
  }
  /* Check we ended where we expected to */
  if (pos != data_len) return -5;
  return -2;
}

int parse_sni(const uint8_t *data, size_t data_len, std::string *sni) {
  return parse_tls_header(data, data_len, sni);
}
}  // namespace snova
