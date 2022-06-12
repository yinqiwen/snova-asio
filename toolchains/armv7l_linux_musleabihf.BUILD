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
            "armv7l-linux-musleabihf/**",
            "usr/include/**",
        ],
    ),
)

filegroup(
    name = "ar",
    srcs = glob(["bin/armv7l-linux-musleabihf-ar"]),
)

filegroup(
    name = "as",
    srcs = glob(["bin/armv7l-linux-musleabihf-as"]),
)

filegroup(
    name = "cpp",
    srcs = glob(["bin/armv7l-linux-musleabihf-cpp"]),
)

filegroup(
    name = "dwp",
    srcs = glob(["bin/armv7l-linux-musleabihf-dwp"]),
)

filegroup(
    name = "gcc",
    srcs = glob(["bin/armv7l-linux-musleabihf-gcc"]),
)

filegroup(
    name = "g++",
    srcs = glob(["bin/armv7l-linux-musleabihf-g++"]),
)

filegroup(
    name = "gcov",
    srcs = glob(["bin/armv7l-linux-musleabihf-gcov"]),
)

filegroup(
    name = "ld",
    srcs = glob(["bin/armv7l-linux-musleabihf-ld"]),
)

filegroup(
    name = "nm",
    srcs = glob(["bin/armv7l-linux-musleabihf-nm"]),
)

filegroup(
    name = "objcopy",
    srcs = glob(["bin/armv7l-linux-musleabihf-objcopy"]),
)

filegroup(
    name = "objdump",
    srcs = glob(["bin/armv7l-linux-musleabihf-objdump"]),
)

filegroup(
    name = "strip",
    srcs = glob(["bin/armv7l-linux-musleabihf-strip"]),
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

# load("@snova_asio//toolchains:armv7l-linux-musleabihf.bzl", "cc_toolchain_config")
load("@snova_asio//toolchains:linux-musl.bzl", "cc_toolchain_config")

# cc_toolchain_suite(
#     name = "buildroot",
#     toolchains = {
#         "armv7": ":armv7-toolchain",
#     },
# )

cc_toolchain_config(
    name = "armv7l-toolchain-config",
    # target_libc = "musl",
    cpu = "armv7l",
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
    toolchain_config = ":armv7l-toolchain-config",
    toolchain_identifier = "armv7l-toolchain",
)
