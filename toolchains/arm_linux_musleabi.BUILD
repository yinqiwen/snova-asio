package(default_visibility = ["//visibility:public"])

filegroup(
    name = "all",
    srcs = glob(
        [
            "bin/**",
            "include/**",
            "lib/**",
            "libexec/**",
            "share/**",
            "arm-linux-musleabi/**",
            "usr/include/**",
        ],
    ),
)

filegroup(
    name = "ar",
    srcs = glob(["bin/arm-linux-musleabi-ar"]),
)

filegroup(
    name = "as",
    srcs = glob(["bin/arm-linux-musleabi-as"]),
)

filegroup(
    name = "cpp",
    srcs = glob(["bin/arm-linux-musleabi-cpp"]),
)

filegroup(
    name = "dwp",
    srcs = glob(["bin/arm-linux-musleabi-dwp"]),
)

filegroup(
    name = "gcc",
    srcs = glob(["bin/arm-linux-musleabi-gcc"]),
)

filegroup(
    name = "g++",
    srcs = glob(["bin/arm-linux-musleabi-g++"]),
)

filegroup(
    name = "gcov",
    srcs = glob(["bin/arm-linux-musleabi-gcov"]),
)

filegroup(
    name = "ld",
    srcs = glob(["bin/arm-linux-musleabi-ld"]),
)

filegroup(
    name = "nm",
    srcs = glob(["bin/arm-linux-musleabi-nm"]),
)

filegroup(
    name = "objcopy",
    srcs = glob(["bin/arm-linux-musleabi-objcopy"]),
)

filegroup(
    name = "objdump",
    srcs = glob(["bin/arm-linux-musleabi-objdump"]),
)

filegroup(
    name = "strip",
    srcs = glob(["bin/arm-linux-musleabi-strip"]),
)

filegroup(
    name = "compiler_components",
    srcs = [
        "ar",
        "as",
        "cpp",
        "dwp",
        "g++",
        "gcc",
        "gcov",
        "ld",
        "nm",
        "objcopy",
        "objdump",
        "strip",
    ],
)

load("@snova_asio//toolchains:linux-musl.bzl", "cc_toolchain_config")

# cc_toolchain_suite(
#     name = "buildroot",
#     toolchains = {
#         "armv7": ":armv7-toolchain",
#     },
# )

cc_toolchain_config(
    name = "arm-toolchain-config",
    # cxx_builtin_include_directories = [
    #     "external/armv7l-linux-musleabihf-cross/include",
    #     "external/armv7l-linux-musleabihf-cross/include/c++/11.2.1/",
    # ],
    # target_libc = "musl",
    cpu = "arm",
    tool_paths = ":compiler_components",
)

cc_toolchain(
    name = "cc_toolchain",
    all_files = ":all",
    ar_files = ":all",
    compiler_files = ":all",
    dwp_files = ":all",
    linker_files = ":all",
    objcopy_files = ":all",
    strip_files = ":all",
    supports_param_files = False,
    toolchain_config = ":arm-toolchain-config",
    toolchain_identifier = "arm-toolchain",
)
