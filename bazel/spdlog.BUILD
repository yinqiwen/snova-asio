package(default_visibility = ["//visibility:public"])

# load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

# filegroup(
#     name = "all_srcs",
#     srcs = glob(["**"]),
#     visibility = ["//visibility:public"],
# )

# cmake(
#     name = "spdlog",
#     generate_args = [
#         # "-DSPDLOG_USE_STD_FORMAT=ON",
#         "-DCMAKE_CXX_STANDARD=20",
#     ],
#     lib_source = ":all_srcs",
#     out_lib_dir = "lib64",
#     # targets = ["spdlog_header_only"],
# )

cc_library(
    name = "spdlog",
    hdrs = glob([
        "include/**/*.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
    # deps = ["@com_github_fmtlib_fmt//:fmtlib"],
)
