"""Sample Starlark definition defining a C++ toolchain's behavior.
When you build a cc_* rule, this logic defines what programs run for what
build steps (e.g. compile / link / archive) and how their command lines are
structured.
This is a proof-of-concept simple implementation. It doesn't construct fancy
command lines and uses mock shell scripts to compile and link
("sample_compiler" and "sample_linker"). See
https://docs.bazel.build/versions/main/cc-toolchain-config-reference.html and
https://docs.bazel.build/versions/main/tutorial/cc-toolchain-config.html for
advanced usage.
"""

load("@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl", "tool_path")

def _impl(ctx):
    out = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.write(out, "Fake executable")
    tool_paths = [
        tool_path(
            name = "ar",
            path = "arm-linux-gnueabihf-ar",
        ),
        tool_path(
            name = "cpp",
            path = "arm-linux-gnueabihf-cpp",
        ),
        tool_path(
            name = "gcc",
            path = "arm-linux-gnueabihf-gcc",
        ),
        tool_path(
            name = "gcov",
            path = "arm-linux-gnueabihf-gcov",
        ),
        tool_path(
            name = "ld",
            path = "arm-linux-gnueabihf-ld",
        ),
        tool_path(
            name = "nm",
            path = "arm-linux-gnueabihf-nm",
        ),
        tool_path(
            name = "objdump",
            path = "arm-linux-gnueabihf-objdump",
        ),
        tool_path(
            name = "strip",
            path = "arm-linux-gnueabihf-strip",
        ),
    ]

    # Documented at
    # https://docs.bazel.build/versions/main/skylark/lib/cc_common.html#create_cc_toolchain_config_info.
    #
    # create_cc_toolchain_config_info is the public interface for registering
    # C++ toolchain behavior.
    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        toolchain_identifier = "custom-toolchain-identifier",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = "sample_cpu",
        target_libc = "unknown",
        compiler = "gcc",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    # You can alternatively define attributes here that make it possible to
    # instantiate different cc_toolchain_config targets with different behavior.
    attrs = {},
    provides = [CcToolchainConfigInfo],
)
