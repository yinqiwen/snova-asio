load(
    "@snova_asio//snova:copts/configure_copts.bzl",
    "SNOVA_DEFAULT_COPTS",
)

licenses(["notice"])

exports_files(["LICENSE.txt"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "nanopb",
    srcs = [
        "pb_common.c",
        "pb_decode.c",
        "pb_encode.c",
    ],
    hdrs = [
        "pb.h",
        "pb_common.h",
        "pb_decode.h",
        "pb_encode.h",
    ],
    copts = SNOVA_DEFAULT_COPTS,
    visibility = ["//visibility:public"],
)
