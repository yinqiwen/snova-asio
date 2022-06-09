package(default_visibility = ["//visibility:public"])

# load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake", "make")

# filegroup(
#     name = "all_srcs",
#     srcs = glob(["**"]),
#     visibility = ["//visibility:public"],
# )

# cmake(
#     name = "mbedtls",
#     generate_args = [
#         "-DCMAKE_BUILD_TYPE=Release",
#         "-DENABLE_PROGRAMS=OFF",
#         "-DENABLE_TESTING=OFF",
#     ],
#     lib_source = ":all_srcs",
#     out_lib_dir = "lib",
#     out_static_libs = select({
#         "@bazel_tools//src/conditions:windows": [
#             "mbedtls.lib",
#             "mbedcrypto.lib",
#             "mbedx509.lib",
#         ],
#         "//conditions:default": [
#             "libmbedtls.a",
#             "libmbedcrypto.a",
#             "libmbedx509.a",
#         ],
#     }),
# )

MBEDTLS_COPTS = select({
    "@bazel_tools//src/conditions:windows": [
        "/O2 /D_FILE_OFFSET_BITS#64",
    ],
    "//conditions:default": [
        "-O2 -Wall -Wextra -Wformat=2 -Wno-format-nonliteral -D_FILE_OFFSET_BITS=64",
    ],
})

cc_library(
    name = "mbedtld_headers",
    hdrs = glob([
        "include/**/*.h",
    ]),
    strip_include_prefix = "include",
)

cc_library(
    name = "mbedtld_src_common_h",
    hdrs = glob([
        "library/*.h",
    ]),
    strip_include_prefix = "library",
)

cc_library(
    name = "libmbedx509",
    srcs = [
        "library/x509.c",
        "library/x509_create.c",
        "library/x509_crl.c",
        "library/x509_crt.c",
        "library/x509_csr.c",
        "library/x509write_crt.c",
        "library/x509write_csr.c",
    ],
    copts = MBEDTLS_COPTS,
    deps = [
        ":mbedtld_headers",
        ":mbedtld_src_common_h",
    ],
)

cc_library(
    name = "libmbedtls",
    srcs = [
        "library/debug.c",
        "library/net_sockets.c",
        "library/ssl_cache.c",
        "library/ssl_ciphersuites.c",
        "library/ssl_cli.c",
        "library/ssl_cookie.c",
        "library/ssl_msg.c",
        "library/ssl_srv.c",
        "library/ssl_ticket.c",
        "library/ssl_tls.c",
        "library/ssl_tls13_client.c",
        "library/ssl_tls13_generic.c",
        "library/ssl_tls13_keys.c",
        "library/ssl_tls13_server.c",
    ],
    copts = MBEDTLS_COPTS,
    deps = [
        ":mbedtld_headers",
        ":mbedtld_src_common_h",
    ],
)

cc_library(
    name = "libmbedcrypto",
    srcs = [
        "library/aes.c",
        "library/aesni.c",
        "library/aria.c",
        "library/asn1parse.c",
        "library/asn1write.c",
        "library/base64.c",
        "library/bignum.c",
        "library/camellia.c",
        "library/ccm.c",
        "library/chacha20.c",
        "library/chachapoly.c",
        "library/cipher.c",
        "library/cipher_wrap.c",
        "library/cmac.c",
        "library/constant_time.c",
        "library/ctr_drbg.c",
        "library/des.c",
        "library/dhm.c",
        "library/ecdh.c",
        "library/ecdsa.c",
        "library/ecjpake.c",
        "library/ecp.c",
        "library/ecp_curves.c",
        "library/entropy.c",
        "library/entropy_poll.c",
        "library/error.c",
        "library/gcm.c",
        "library/hkdf.c",
        "library/hmac_drbg.c",
        "library/md.c",
        "library/md5.c",
        "library/memory_buffer_alloc.c",
        "library/mps_reader.c",
        "library/mps_trace.c",
        "library/nist_kw.c",
        "library/oid.c",
        "library/padlock.c",
        "library/pem.c",
        "library/pk.c",
        "library/pk_wrap.c",
        "library/pkcs12.c",
        "library/pkcs5.c",
        "library/pkparse.c",
        "library/pkwrite.c",
        "library/platform.c",
        "library/platform_util.c",
        "library/poly1305.c",
        "library/psa_crypto.c",
        "library/psa_crypto_aead.c",
        "library/psa_crypto_cipher.c",
        "library/psa_crypto_client.c",
        "library/psa_crypto_driver_wrappers.c",
        "library/psa_crypto_ecp.c",
        "library/psa_crypto_hash.c",
        "library/psa_crypto_mac.c",
        "library/psa_crypto_rsa.c",
        "library/psa_crypto_se.c",
        "library/psa_crypto_slot_management.c",
        "library/psa_crypto_storage.c",
        "library/psa_its_file.c",
        "library/ripemd160.c",
        "library/rsa.c",
        "library/rsa_alt_helpers.c",
        "library/sha1.c",
        "library/sha256.c",
        "library/sha512.c",
        "library/ssl_debug_helpers_generated.c",
        "library/threading.c",
        "library/timing.c",
        "library/version.c",
        "library/version_features.c",
    ],
    copts = MBEDTLS_COPTS,
    deps = [
        ":mbedtld_headers",
        ":mbedtld_src_common_h",
    ],
)

cc_library(
    name = "mbedtls",
    deps = [
        ":libmbedcrypto",
        ":libmbedtls",
        ":libmbedx509",
    ],
)
