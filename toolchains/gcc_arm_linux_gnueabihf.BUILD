package(default_visibility = ["//visibility:public"])

filegroup(
    name = "all",
    srcs = glob(["**/**"]),
    # srcs = [],
)

filegroup(
    name = "ar",
    srcs = glob(["bin/arm-none-linux-gnueabihf-ar"]),
)

filegroup(
    name = "as",
    srcs = glob(["bin/arm-none-linux-gnueabihf-as"]),
)

filegroup(
    name = "cpp",
    srcs = glob(["bin/arm-none-linux-gnueabihf-cpp"]),
)

filegroup(
    name = "dwp",
    srcs = glob(["bin/arm-none-linux-gnueabihf-dwp"]),
)

filegroup(
    name = "gcc",
    srcs = glob(["bin/arm-none-linux-gnueabihf-gcc"]),
)

filegroup(
    name = "g++",
    srcs = glob(["bin/arm-none-linux-gnueabihf-g++"]),
)

filegroup(
    name = "gcov",
    srcs = glob(["bin/arm-none-linux-gnueabihf-gcov"]),
)

filegroup(
    name = "ld",
    srcs = glob(["bin/arm-none-linux-gnueabihf-ld"]),
)

filegroup(
    name = "nm",
    srcs = glob(["bin/arm-none-linux-gnueabihf-nm"]),
)

filegroup(
    name = "objcopy",
    srcs = glob(["bin/arm-none-linux-gnueabihf-objcopy"]),
)

filegroup(
    name = "objdump",
    srcs = glob(["bin/arm-none-linux-gnueabihf-objdump"]),
)

filegroup(
    name = "strip",
    srcs = glob(["bin/arm-none-linux-gnueabihf-strip"]),
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
        # "-mthumb",
        # "-mcpu=cortex-a9",
    ],
    compiler = "gcc",
    cpp_version = "c++2a",
    cpu = "armv7a",
    cxx_builtin_include_directories = [
    ],
    dbg_compile_flags = [],
    link_flags = [
        # "--specs=rdimon.specs",
        # "-lrdimon",
        # "-mcpu=cortex-a9",
        #"-march=armv7-a",
        # "-mthumb",
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
