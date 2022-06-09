package(default_visibility = ["//visibility:public"])

platform(
    name = "linux_arm",
    constraint_values = [
        "@platforms//cpu:armv7",
        "@platforms//os:linux",
    ],
)

toolchain(
    name = "gcc_arm_none_linux_gnueabihf_compile_toolchain",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    target_compatible_with = [
        "@platforms//cpu:armv7",
        "@platforms//os:linux",
    ],
    toolchain = "@gcc_arm_linux_gnueabihf//:cc_toolchain",
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)
