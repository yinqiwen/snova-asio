package(default_visibility = ["//visibility:public"])

constraint_setting(name = "libc")

constraint_value(
    name = "musl",
    constraint_setting = ":libc",
)

constraint_setting(name = "abi")

constraint_value(
    name = "eabi",
    constraint_setting = ":abi",
)

constraint_value(
    name = "eabihf",
    constraint_setting = ":abi",
)

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

platform(
    name = "linux_musl_arm_eabi",
    constraint_values = [
        "@platforms//cpu:arm",
        "@platforms//os:linux",
        ":eabi",
    ],
)

platform(
    name = "linux_musl_arm_eabihf",
    constraint_values = [
        "@platforms//cpu:arm",
        "@platforms//os:linux",
        ":eabihf",
    ],
)

platform(
    name = "linux_musl_aarch64",
    constraint_values = [
        "@platforms//cpu:aarch64",
        "@platforms//os:linux",
    ],
)

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

toolchain(
    name = "arm_linux_musleabi_gcc_compile_toolchain",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    target_compatible_with = [
        "@platforms//cpu:arm",
        "@platforms//os:linux",
        ":eabi",
    ],
    toolchain = select({
        "//:local_musl": "@local_arm_linux_musleabi//:cc_toolchain",
        "//conditions:default": "@arm_linux_musleabi//:cc_toolchain",
    }),
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)

toolchain(
    name = "arm_linux_musleabihf_gcc_compile_toolchain",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    target_compatible_with = [
        "@platforms//cpu:arm",
        "@platforms//os:linux",
        ":eabihf",
    ],
    toolchain = select({
        "//:local_musl": "@local_arm_linux_musleabihf//:cc_toolchain",
        "//conditions:default": "@arm_linux_musleabihf//:cc_toolchain",
    }),
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)

toolchain(
    name = "aarch64_linux_musl_gcc_compile_toolchain",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    target_compatible_with = [
        "@platforms//cpu:aarch64",
        "@platforms//os:linux",
    ],
    toolchain = select({
        "//:local_musl": "@local_aarch64_linux_musl_gcc//:cc_toolchain",
        "//conditions:default": "@aarch64_linux_musl//:cc_toolchain",
    }),
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)
