package(default_visibility = ["//visibility:public"])

constraint_setting(name = "libc")

constraint_value(
    name = "musl",
    constraint_setting = ":libc",
)

# platform(
#     name = "linux_arm",
#     constraint_values = [
#         "@platforms//cpu:armv7",
#         "@platforms//os:linux",
#     ],
# )

platform(
    name = "linux_musl_x64",
    constraint_values = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
        ":musl",
    ],
)

constraint_value(
    name = "armv5l",
    constraint_setting = "@platforms//cpu",
)

constraint_value(
    name = "armv7l",
    constraint_setting = "@platforms//cpu",
)

platform(
    name = "linux_musl_armv7l",
    constraint_values = [
        # "@platforms//cpu:armv7",
        ":armv7l",
        "@platforms//os:linux",
        ":musl",
    ],
)

platform(
    name = "linux_musl_armv5l",
    constraint_values = [
        # "@platforms//cpu:armv7",
        ":armv5l",
        "@platforms//os:linux",
        ":musl",
    ],
)

# toolchain(
#     name = "gcc_arm_none_linux_gnueabihf_compile_toolchain",
#     exec_compatible_with = [
#         "@platforms//cpu:x86_64",
#         "@platforms//os:linux",
#     ],
#     target_compatible_with = [
#         "@platforms//cpu:armv7",
#         "@platforms//os:linux",
#         ":musl",
#     ],
#     toolchain = "@gcc_arm_linux_gnueabihf//:cc_toolchain",
#     toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
# )

toolchain(
    name = "x64_linux_musl_gcc_compile_toolchain",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    target_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    # toolchain = "@x64_linux_musl_gcc//:cc_toolchain",
    toolchain = select({
        "//:local_musl": "@local_x64_linux_musl_gcc//:cc_toolchain",
        "//conditions:default": "@x64_linux_musl_gcc//:cc_toolchain",
    }),
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)

# toolchain_type(name = "armv7l_linux_musleabihf_toolchain_type")

toolchain(
    name = "armv7l_linux_musleabihf_gcc_compile_toolchain",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    target_compatible_with = [
        # "@platforms//cpu:armv7",
        ":armv7l",
        "@platforms//os:linux",
    ],
    toolchain = select({
        "//:local_musl": "@local_armv7l_linux_musleabihf//:cc_toolchain",
        "//conditions:default": "@armv7l_linux_musleabihf//:cc_toolchain",
    }),
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)

toolchain(
    name = "armv5l_linux_musleabi_gcc_compile_toolchain",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    target_compatible_with = [
        # "@platforms//cpu:armv7",
        ":armv5l",
        "@platforms//os:linux",
    ],
    toolchain = select({
        "//:local_musl": "@local_armv5l_linux_musleabi//:cc_toolchain",
        "//conditions:default": "@armv5l_linux_musleabi//:cc_toolchain",
    }),
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)
