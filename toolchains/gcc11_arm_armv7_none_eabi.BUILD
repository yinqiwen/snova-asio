package(default_visibility = ["//visibility:public"])

filegroup(
    name = "all",
    srcs = glob(["**/**"]),
)

filegroup(
    name = "ar",
    srcs = glob(["bin/*-ar"]),
)

filegroup(
    name = "as",
    srcs = glob(["bin/*-as"]),
)

filegroup(
    name = "cpp",
    srcs = glob(["bin/*-cpp"]),
)

filegroup(
    name = "dwp",
    srcs = glob(["bin/*-dwp"]),
)

filegroup(
    name = "gcc",
    srcs = glob(["bin/*-gcc"]),
)

filegroup(
    name = "g++",
    srcs = glob(["bin/*-g++"]),
)

filegroup(
    name = "gcov",
    srcs = glob(["bin/*-gcov"]),
)

filegroup(
    name = "ld",
    srcs = glob(["bin/*-ld"]),
)

filegroup(
    name = "nm",
    srcs = glob(["bin/*-nm"]),
)

filegroup(
    name = "objcopy",
    srcs = glob(["bin/*-objcopy"]),
)

filegroup(
    name = "objdump",
    srcs = glob(["bin/*-objdump"]),
)

filegroup(
    name = "strip",
    srcs = glob(["bin/*-strip"]),
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

load("@snova_asio//toolchains:gcc_arm_toolchain.bzl", "cc_toolchain_config")

cc_toolchain_config(
    name = "toolchain_config",
    abi_libc_version = "unknown",
    abi_version = "eabi",
    c_version = "c99",
    compile_flags = [
        "-mthumb",
        "-mcpu=cortex-a9",
    ],
    compiler = "gcc",
    cpp_version = "c++2a",
    cpu = "armv7a",
    cxx_builtin_include_directories = [
    ],
    dbg_compile_flags = [],
    link_flags = [
        "--specs=rdimon.specs",
        "-lrdimon",
        "-mcpu=cortex-a9",
        #"-march=armv7-a",
        "-mthumb",
    ],
    link_libs = [],
    opt_compile_flags = [],
    opt_link_flags = [],
    target_libc = "unknown",
    tool_paths = ":compiler_components",
    unfiltered_compile_flags = [],
)

cc_toolchain(
    name = "cc_toolchain",
    all_files = ":all",
    ar_files = ":all",
    as_files = ":all",
    compiler_files = ":all",
    dwp_files = ":all",
    linker_files = ":all",
    objcopy_files = ":all",
    strip_files = ":all",
    supports_param_files = 0,
    toolchain_config = ":toolchain_config",
)
