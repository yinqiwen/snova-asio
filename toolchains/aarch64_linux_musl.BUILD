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
            "aarch64-linux-musl/**",
            "usr/include/**",
        ],
    ),
)

filegroup(
    name = "ar",
    srcs = glob(["bin/aarch64-linux-musl-ar"]),
)

filegroup(
    name = "as",
    srcs = glob(["bin/aarch64-linux-musl-as"]),
)

filegroup(
    name = "cpp",
    srcs = glob(["bin/aarch64-linux-musl-cpp"]),
)

filegroup(
    name = "dwp",
    srcs = glob(["bin/aarch64-linux-musl-dwp"]),
)

filegroup(
    name = "gcc",
    srcs = glob(["bin/aarch64-linux-musl-gcc"]),
)

filegroup(
    name = "g++",
    srcs = glob(["bin/aarch64-linux-musl-g++"]),
)

filegroup(
    name = "gcov",
    srcs = glob(["bin/aarch64-linux-musl-gcov"]),
)

filegroup(
    name = "ld",
    srcs = glob(["bin/aarch64-linux-musl-ld"]),
)

filegroup(
    name = "nm",
    srcs = glob(["bin/aarch64-linux-musl-nm"]),
)

filegroup(
    name = "objcopy",
    srcs = glob(["bin/aarch64-linux-musl-objcopy"]),
)

filegroup(
    name = "objdump",
    srcs = glob(["bin/aarch64-linux-musl-objdump"]),
)

filegroup(
    name = "strip",
    srcs = glob(["bin/aarch64-linux-musl-strip"]),
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
    name = "aarch64-toolchain-config",
    # cxx_builtin_include_directories = [
    #     "external/armv7l-linux-musleabihf-cross/include",
    #     "external/armv7l-linux-musleabihf-cross/include/c++/11.2.1/",
    # ],
    # target_libc = "musl",
    cpu = "aarch64",
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
    toolchain_config = ":aarch64-toolchain-config",
    toolchain_identifier = "aarch64-toolchain",
)
