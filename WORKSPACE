workspace(name = "snova_asio")

load("//:snova_asio.bzl", "snova_workspace")

snova_workspace()

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

# Don't use preinstalled tools to ensure builds are as hermetic as possible
rules_foreign_cc_dependencies()

register_toolchains(
    "//toolchains:x64_linux_musl_gcc_compile_toolchain",
    "//toolchains:arm_linux_musleabi_gcc_compile_toolchain",
    "//toolchains:arm_linux_musleabihf_gcc_compile_toolchain",
    "//toolchains:aarch64_linux_musl_gcc_compile_toolchain",
)

# register_execution_platforms("//toolchains:linux_armv7")
