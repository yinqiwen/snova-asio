load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "all_srcs",
    srcs = glob(["**"]),
)

cmake(
    name = "mimalloc",
    generate_args = [
        "-DCMAKE_BUILD_TYPE=Release",
        "-DMI_BUILD_SHARED=OFF",
        "-DMI_BUILD_TESTS=OFF",
    ],
    lib_source = ":all_srcs",
    out_binaries = select({
        "@bazel_tools//src/conditions:windows": ["../lib/mimalloc-2.0/mimalloc.obj"],
        "//conditions:default": ["../lib64/mimalloc-2.0/mimalloc.o"],
    }),
    out_lib_dir = select({
        "@bazel_tools//src/conditions:windows": "lib",
        "//conditions:default": "lib64",
    }),
    out_static_libs = select({
        "@bazel_tools//src/conditions:windows": [
            "mimalloc-2.0/mimalloc-static.lib",
        ],
        "//conditions:default": [
            "mimalloc-2.0/libmimalloc.a",
        ],
    }),
    visibility = ["//visibility:public"],
)

# genrule(
#     name = "gen_mimalloc_override",
#     srcs = [
#         ":mimalloc",
#     ],
#     outs = ["mimalloc_override.lib"],
#     cmd = " \n ".join([
#         "for filename in $(locations @com_github_microsoft_mimalloc//:mimalloc);do",
#         "if [[ $$filename == *'mimalloc.o' || $$filename == *'mimalloc.obj' ]]; then",
#         "ar -crs $@ $$filename ",
#         "fi",
#         "done",
#     ]),
#     visibility = ["//visibility:public"],
# )

# cc_import(
#     name = "mimalloc_override_import",
#     static_library = ":gen_mimalloc_override",
#     visibility = ["//visibility:public"],
#     alwayslink = 1,
# )

# cc_library(
#     name = "mimalloc_override",
#     visibility = ["//visibility:public"],
#     deps = [
#         ":mimalloc_override_import",
#     ],
# )
