workspace(name = "snova_asio")

load("//:snova_asio.bzl", "snova_workspace")

snova_workspace()

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

# Don't use preinstalled tools to ensure builds are as hermetic as possible
rules_foreign_cc_dependencies()

register_toolchains(
    "//toolchains:gcc_arm_none_linux_gnueabihf_compile_toolchain",
)

# register_execution_platforms("//toolchains:linux_armv7")
