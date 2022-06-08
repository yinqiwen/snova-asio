package(default_visibility = ["//visibility:public"])

load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
    visibility = ["//visibility:public"],
)

configure_make(
    name = "jemalloc_build",
    args = ["-j 6"],
    autogen = True,
    configure_in_place = True,
    configure_options = [
        # "--enable-prof",
        # "--enable-prof-libunwind",
        "--disable-libdl",
    ],
    lib_source = ":all_srcs",
    out_static_libs = select({
        "@bazel_tools//src/conditions:windows": [
            "jemalloc.lib",
        ],
        "//conditions:default": [
            "libjemalloc.a",
        ],
    }),
    # out_binaries = ["jeprof"],
    targets = [
        "build_lib_static",
        "install_include",
        "install_lib_static",
    ],
)

cc_library(
    name = "jemalloc",
    tags = ["no-cache"],
    deps = [
        ":jemalloc_build",
    ],
)
