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
            "x86_64-linux-musl/**",
            "usr/include/**",
        ],
    ),
)

filegroup(
    name = "ar",
    srcs = glob(["bin/ar"]),
)

filegroup(
    name = "as",
    srcs = glob(["bin/as"]),
)

filegroup(
    name = "cpp",
    srcs = glob(["bin/cpp"]),
)

filegroup(
    name = "dwp",
    srcs = glob(["bin/dwp"]),
)

filegroup(
    name = "gcc",
    srcs = glob(["bin/x86_64-linux-musl-gcc"]),
)

filegroup(
    name = "g++",
    srcs = glob(["bin/x86_64-linux-musl-g++"]),
)

filegroup(
    name = "gcov",
    srcs = glob(["bin/gcov"]),
)

filegroup(
    name = "ld",
    srcs = glob(["bin/ld"]),
)

filegroup(
    name = "nm",
    srcs = glob(["bin/nm"]),
)

filegroup(
    name = "objcopy",
    srcs = glob(["bin/objcopy"]),
)

filegroup(
    name = "objdump",
    srcs = glob(["bin/objdump"]),
)

filegroup(
    name = "strip",
    srcs = glob(["bin/strip"]),
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
#         "x86_64": ":x86_64-toolchain",
#     },
# )

cc_toolchain_config(
    name = "x86_64-toolchain-config",
    # target_libc = "musl",
    cpu = "k8",
    cxx_builtin_include_directories = [
        "include",
    ],
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
    toolchain_config = ":x86_64-toolchain-config",
    toolchain_identifier = "k8-toolchain",
)
