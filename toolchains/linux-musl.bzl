load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "feature",
    "flag_group",
    "flag_set",
    "tool_path",
)

all_compile_actions = [
    ACTION_NAMES.assemble,
    ACTION_NAMES.c_compile,
    ACTION_NAMES.clif_match,
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.cpp_header_parsing,
    ACTION_NAMES.cpp_module_codegen,
    ACTION_NAMES.cpp_module_compile,
    ACTION_NAMES.linkstamp_compile,
    ACTION_NAMES.lto_backend,
    ACTION_NAMES.preprocess_assemble,
]

all_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

target_system_name = {
    "k8": "x86_64-linux-gnu",
    "armv7l": "armv7l-linux-musleabihf",
    "armv5l": "armv5l-linux-musleabi",
}

def _toolchain_config_impl(ctx):
    support_tools = ["ar", "as", "compat-ld", "cpp", "dwp", "gcc", "g++", "gcov", "ld", "nm", "objcopy", "objdump", "strip"]
    tool_paths = []
    for k in ctx.attr.tool_paths.files.to_list():
        name = k.basename.split("-")[-1]  # Attempts to get toolname
        if name in support_tools:
            tool_paths.append(tool_path(name = name, path = "/".join(k.path.split("/")[2:])))
        else:
            fail("Not a tool: {} \nParsed from: {}".format(name, k.basename))

    unfiltered_compile_flags_feature = feature(
        name = "unfiltered_compile_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = all_compile_actions,
                flag_groups = [
                    flag_group(
                        flags = [
                            # These make sure that paths included in .d dependency files are relative to the execroot
                            # (e.g. start with "external/").
                            "-no-canonical-prefixes",
                            "-fno-canonical-system-headers",
                        ],
                    ),
                ],
            ),
        ],
    )

    dbg_feature = feature(name = "dbg")
    opt_feature = feature(name = "opt")

    default_link_flags_feature = feature(
        name = "default_link_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = all_link_actions,
                flag_groups = [
                    flag_group(
                        flags = [
                            "-Wl,-no-as-needed",
                            "-Wl,-z,relro,-z,now",
                            "-lstdc++",
                            "-lm",
                            "-pass-exit-codes",
                        ],
                    ),
                ],
            ),
        ],
    )

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        toolchain_identifier = ctx.attr.cpu + "-toolchain",
        host_system_name = "x86_64-unknown-linux-gnu",
        # target_system_name = "x86_64-buildroot-linux-musl",
        target_system_name = target_system_name[ctx.attr.cpu],
        cxx_builtin_include_directories = ctx.attr.cxx_builtin_include_directories,
        target_cpu = ctx.attr.cpu,
        target_libc = "musl",
        compiler = "gcc",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
        features = [
            default_link_flags_feature,
            unfiltered_compile_flags_feature,
        ],
    )

cc_toolchain_config = rule(
    implementation = _toolchain_config_impl,
    attrs = {
        "tool_paths": attr.label(mandatory = True, allow_files = True),
        "cxx_builtin_include_directories": attr.string_list(),
        "cpu": attr.string(mandatory = True, values = ["armv7l", "armv5l", "k8"]),
    },
    provides = [CcToolchainConfigInfo],
)
