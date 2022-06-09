package(default_visibility = ["//visibility:public"])

platform(
    name = "linux_armv7",
    constraint_values = [
        "@platforms//cpu:armv7",
        "@platforms//os:linux",
        # ":gcc_11",
        # ":musleabi",
    ],
)

toolchain(
    name = "gcc_arm_armv7_none_eabi_xcompile_toolchain",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    target_compatible_with = [
        "@platforms//cpu:armv7",
        "@platforms//os:linux",
    ],
    toolchain = "@gcc11_arm_armv7_none_eabi//:cc_toolchain",
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)
