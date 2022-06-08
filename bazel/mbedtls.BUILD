package(default_visibility = ["//visibility:public"])

load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)

cmake(
    name = "mbedtls",
    generate_args = [
        "-DCMAKE_BUILD_TYPE=Release",
        "-DENABLE_PROGRAMS=OFF",
        "-DENABLE_TESTING=OFF",
    ],
    lib_source = ":all_srcs",
    out_lib_dir = "lib",
    out_static_libs = select({
        "@bazel_tools//src/conditions:windows": [
            "mbedtls.lib",
            "mbedcrypto.lib",
            "mbedx509.lib",
        ],
        "//conditions:default": [
            "libmbedtls.a",
            "libmbedcrypto.a",
            "libmbedx509.a",
        ],
    }),
)
