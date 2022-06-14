load(
    "@bazel_tools//tools/cpp:cc_toolchain_config_lib.bzl",
    "action_config",
    "artifact_name_pattern",
    "feature",
    "feature_set",
    "flag_group",
    "flag_set",
    "tool",
    "tool_path",
    "variable_with_value",
    "with_feature_set",
)
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")

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

all_cpp_compile_actions = [
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.cpp_header_parsing,
    ACTION_NAMES.cpp_module_compile,
    ACTION_NAMES.cpp_module_codegen,
    ACTION_NAMES.clif_match,
    ACTION_NAMES.linkstamp_compile,
]

preprocessor_compile_actions = [
    ACTION_NAMES.c_compile,
    ACTION_NAMES.clif_match,
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.cpp_header_parsing,
    ACTION_NAMES.cpp_module_compile,
    ACTION_NAMES.linkstamp_compile,
    ACTION_NAMES.preprocess_assemble,
]

codegen_compile_actions = [
    ACTION_NAMES.assemble,
    ACTION_NAMES.c_compile,
    ACTION_NAMES.cpp_compile,
    ACTION_NAMES.cpp_module_codegen,
    ACTION_NAMES.linkstamp_compile,
    ACTION_NAMES.lto_backend,
    ACTION_NAMES.preprocess_assemble,
]

all_link_actions = [
    ACTION_NAMES.cpp_link_executable,
    ACTION_NAMES.cpp_link_dynamic_library,
    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
]

lto_index_actions = [
    ACTION_NAMES.lto_index_for_executable,
    ACTION_NAMES.lto_index_for_dynamic_library,
    ACTION_NAMES.lto_index_for_nodeps_dynamic_library,
]

target_system_name = {
    "k8": "x86_64-linux-gnu",
    "armv7a": "arm-linux-gnueabihf",
    "armv6": "arm-linux-gnueabihf",
    "aarch64": "aarch64-linux-gnu",
    "riscv64": "riscv64-linux-gnu",
}

compile_flags = {
    "k8": ["-msse4.2"],
    "armv7a": [],
    "armv6": [],
    "aarch64": [],
    "riscv64": [],
}

common_opt_compile_flags = [
    "-g0",
    "-O3",
    "-DNDEBUG",
    "-D_FORTIFY_SOURCE=2",
    "-ffunction-sections",
    "-fdata-sections",
]

non_k8_opt_compile_flags = [
    #"-funsafe-math-optimizations",
    #"-ftree-vectorize",
]

opt_compile_flags = {
    "k8": common_opt_compile_flags,
    "armv7a": common_opt_compile_flags + non_k8_opt_compile_flags,
    "armv6": common_opt_compile_flags + non_k8_opt_compile_flags,
    "aarch64": common_opt_compile_flags + non_k8_opt_compile_flags,
    "riscv64": common_opt_compile_flags + non_k8_opt_compile_flags,
}

common_linker_flags = [
    "-lstdc++",
    "-Wl,-z,relro,-z,now",
    "-pass-exit-codes",
    "-lm",
    "-Wall",
]

target_linker_flags = {
    "k8": [],
    "armv7a": [],
    "armv6": [],
    "aarch64": [],
    "riscv64": [],
}

def _impl(ctx):
    print("xiedeacc:%{preprocessor_defines}")
    preprocessor_defines_feature = feature(
        name = "preprocessor_defines",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = preprocessor_compile_actions,
                flag_groups = [
                    flag_group(
                        flags = ["-D%{preprocessor_defines}"],
                        iterate_over = "preprocessor_defines",
                    ),
                ],
            ),
        ],
    )

    includes_feature = feature(
        name = "includes",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = preprocessor_compile_actions + [
                    ACTION_NAMES.objc_compile,
                    ACTION_NAMES.objcpp_compile,
                ],
                flag_groups = [
                    flag_group(
                        flags = ["-include", "%{includes}"],
                        iterate_over = "includes",
                        expand_if_available = "includes",
                    ),
                ],
            ),
        ],
    )

    print("xiedeacc:%{system_include_paths}")
    include_paths_feature = feature(
        name = "include_paths",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = preprocessor_compile_actions + [
                    ACTION_NAMES.objc_compile,
                    ACTION_NAMES.objcpp_compile,
                ],
                flag_groups = [
                    flag_group(
                        flags = ["-iquote", "%{quote_include_paths}"],
                        iterate_over = "quote_include_paths",
                    ),
                    flag_group(
                        flags = ["-I%{include_paths}"],
                        iterate_over = "include_paths",
                    ),
                    flag_group(
                        flags = ["-isystem", "%{system_include_paths}"],
                        iterate_over = "system_include_paths",
                    ),
                ],
            ),
        ],
    )

    default_compile_flags_feature = feature(
        name = "default_compile_flags",
        enabled = True,
        flag_sets = [
            # default compile option
            flag_set(
                actions = all_compile_actions,
                flag_groups = [
                    flag_group(
                        flags = compile_flags[ctx.attr.cpu] + [
                            "-fPIC",
                            "-fstack-protector",
                            "-Wall",
                            "-fno-omit-frame-pointer",
                        ],
                    ),
                ],
            ),
            # user compile option
            flag_set(
                actions = all_compile_actions,
                flag_groups = ([
                    flag_group(
                        flags = ctx.attr.compile_flags,
                    ),
                ] if ctx.attr.compile_flags else []),
            ),
            # user dbg compile option
            flag_set(
                actions = all_compile_actions,
                flag_groups = ([
                    flag_group(
                        flags = ctx.attr.dbg_compile_flags,
                    ),
                ] if ctx.attr.dbg_compile_flags else []),
                with_features = [with_feature_set(features = ["dbg"])],
            ),
            # user opt compile option
            flag_set(
                actions = all_compile_actions,
                flag_groups = ([
                    flag_group(
                        flags = ctx.attr.opt_compile_flags + opt_compile_flags[ctx.attr.cpu],
                    ),
                ] if ctx.attr.opt_compile_flags else []),
                with_features = [with_feature_set(features = ["opt"])],
            ),
            # if user specify -std, use user defined, dafault c++14
            flag_set(
                actions = all_cpp_compile_actions + [ACTION_NAMES.lto_backend],
                flag_groups = [
                    flag_group(
                        flags = [
                            "-std=" + ctx.attr.cpp_version,
                        ],
                    ),
                ],
            ),
            # if user specify -std, use user defined, dafault c99
            flag_set(
                actions = [
                    ACTION_NAMES.c_compile,
                ],
                flag_groups = [
                    flag_group(
                        flags = [
                            "-std=" + ctx.attr.c_version,
                        ],
                    ),
                ],
            ),
        ],
    )

    unfiltered_compile_flags_feature = feature(
        name = "unfiltered_compile_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = all_compile_actions,
                flag_groups = [
                    flag_group(
                        flags = [
                            "-no-canonical-prefixes",
                            "-fno-canonical-system-headers",
                            "-Wno-builtin-macro-redefined",
                            "-D__DATE__=\"redacted\"",
                            "-D__TIMESTAMP__=\"redacted\"",
                            "-D__TIME__=\"redacted\"",
                        ],
                    ),
                ],
            ),
            flag_set(
                actions = all_compile_actions,
                flag_groups = ([
                    flag_group(
                        flags = ctx.attr.unfiltered_compile_flags,
                    ),
                ] if ctx.attr.unfiltered_compile_flags else []),
            ),
        ],
    )

    compiler_input_flags_feature = feature(
        name = "compiler_input_flags",
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.assemble,
                    ACTION_NAMES.preprocess_assemble,
                    ACTION_NAMES.c_compile,
                    ACTION_NAMES.cpp_compile,
                    ACTION_NAMES.cpp_header_parsing,
                    ACTION_NAMES.cpp_module_compile,
                    ACTION_NAMES.cpp_module_codegen,
                ],
                flag_groups = [
                    flag_group(
                        flags = ["-c", "%{source_file}"],
                        expand_if_available = "source_file",
                    ),
                ],
            ),
        ],
    )

    user_compile_flags_feature = feature(
        name = "user_compile_flags",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = all_compile_actions,
                flag_groups = [
                    flag_group(
                        flags = ["%{user_compile_flags}"],
                        iterate_over = "user_compile_flags",
                        expand_if_available = "user_compile_flags",
                    ),
                ],
            ),
        ],
    )

    library_search_directories_feature = feature(
        name = "library_search_directories",
        flag_sets = [
            flag_set(
                actions = all_link_actions + lto_index_actions,
                flag_groups = [
                    flag_group(
                        flags = ["-L%{library_search_directories}"],
                        iterate_over = "library_search_directories",
                        expand_if_available = "library_search_directories",
                    ),
                ],
            ),
        ],
    )

    runtime_library_search_directories_feature = feature(
        name = "runtime_library_search_directories",
        flag_sets = [
            flag_set(
                actions = all_link_actions + lto_index_actions,
                flag_groups = [
                    flag_group(
                        iterate_over = "runtime_library_search_directories",
                        flag_groups = [
                            flag_group(
                                flags = [
                                    "-Wl,-rpath,$EXEC_ORIGIN/%{runtime_library_search_directories}",
                                ],
                                expand_if_true = "is_cc_test",
                            ),
                            flag_group(
                                flags = [
                                    "-Wl,-rpath,$ORIGIN/%{runtime_library_search_directories}",
                                ],
                                expand_if_false = "is_cc_test",
                            ),
                        ],
                        expand_if_available =
                            "runtime_library_search_directories",
                    ),
                ],
                with_features = [
                    with_feature_set(features = ["static_link_cpp_runtimes"]),
                ],
            ),
            flag_set(
                actions = all_link_actions + lto_index_actions,
                flag_groups = [
                    flag_group(
                        iterate_over = "runtime_library_search_directories",
                        flag_groups = [
                            flag_group(
                                flags = [
                                    "-Wl,-rpath,$ORIGIN/%{runtime_library_search_directories}",
                                ],
                            ),
                        ],
                        expand_if_available =
                            "runtime_library_search_directories",
                    ),
                ],
                with_features = [
                    with_feature_set(
                        not_features = ["static_link_cpp_runtimes"],
                    ),
                ],
            ),
        ],
    )

    default_link_flags_feature = feature(
        name = "default_link_flags",
        enabled = True,
        flag_sets = [
            # default link option
            flag_set(
                actions = all_link_actions + lto_index_actions,
                flag_groups = [
                    flag_group(
                        flags = common_linker_flags + target_linker_flags[ctx.attr.cpu],
                    ),
                ],
            ),
            # user link option
            flag_set(
                actions = all_link_actions + lto_index_actions,
                flag_groups = ([
                    flag_group(
                        flags = ctx.attr.link_flags,
                    ),
                ] if ctx.attr.link_flags else []),
            ),
            # default opt linker option
            flag_set(
                actions = all_link_actions + lto_index_actions,
                flag_groups = [
                    flag_group(flags = ["-Wl,--gc-sections"]),
                ],
                with_features = [with_feature_set(features = ["opt"])],
            ),
            # user opt linker option
            flag_set(
                actions = all_link_actions + lto_index_actions,
                flag_groups = ([
                    flag_group(
                        flags = ctx.attr.opt_link_flags,
                    ),
                ] if ctx.attr.opt_link_flags else []),
                with_features = [with_feature_set(features = ["opt"])],
            ),
        ],
    )

    static_libgcc_feature = feature(
        name = "static_libgcc",
        enabled = True,
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.cpp_link_executable,
                    ACTION_NAMES.cpp_link_dynamic_library,
                    ACTION_NAMES.lto_index_for_executable,
                    ACTION_NAMES.lto_index_for_dynamic_library,
                ],
                flag_groups = [flag_group(flags = ["-static-libgcc"])],
                with_features = [
                    with_feature_set(features = ["static_link_cpp_runtimes"]),
                ],
            ),
        ],
    )

    shared_flag_feature = feature(
        name = "shared_flag",
        flag_sets = [
            flag_set(
                actions = [
                    ACTION_NAMES.cpp_link_dynamic_library,
                    ACTION_NAMES.cpp_link_nodeps_dynamic_library,
                    ACTION_NAMES.lto_index_for_dynamic_library,
                    ACTION_NAMES.lto_index_for_nodeps_dynamic_library,
                ],
                flag_groups = [flag_group(flags = ["-shared"])],
            ),
        ],
    )

    user_link_flags_feature = feature(
        name = "user_link_flags",
        flag_sets = [
            flag_set(
                actions = all_link_actions + lto_index_actions,
                flag_groups = [
                    flag_group(
                        flags = ["%{user_link_flags}"],
                        iterate_over = "user_link_flags",
                        expand_if_available = "user_link_flags",
                    ),
                ] + ([flag_group(flags = ctx.attr.link_libs)] if ctx.attr.link_libs else []),
            ),
        ],
    )

    support_tools = ["ar", "as", "compat-ld", "cpp", "dwp", "gcc", "g++", "gcov", "ld", "nm", "objcopy", "objdump", "strip"]
    tool_paths = []
    for k in ctx.attr.tool_paths.files.to_list():
        name = k.basename.split("-")[-1]  # Attempts to get toolname
        if name in support_tools:
            tool_paths.append(tool_path(name = name, path = "/".join(k.path.split("/")[2:])))
        else:
            fail("Not a tool: {} \nParsed from: {}".format(name, k.basename))

    opt_feature = feature(name = "opt")
    dbg_feature = feature(name = "dbg")
    supports_dynamic_linker_feature = feature(name = "supports_dynamic_linker", enabled = True)
    static_linking_mode_feature = feature(name = "static_linking_mode")
    fully_static_link_feature = feature(name = "fully_static_link")
    dynamic_linking_mode_feature = feature(name = "dynamic_linking_mode")

    features = [
        preprocessor_defines_feature,
        includes_feature,
        include_paths_feature,
        default_compile_flags_feature,
        unfiltered_compile_flags_feature,
        compiler_input_flags_feature,
        user_compile_flags_feature,
        library_search_directories_feature,
        runtime_library_search_directories_feature,
        default_link_flags_feature,
        static_libgcc_feature,
        shared_flag_feature,
        user_link_flags_feature,
        opt_feature,
        dbg_feature,
        supports_dynamic_linker_feature,
        static_linking_mode_feature,
        fully_static_link_feature,
        dynamic_linking_mode_feature,
    ]

    print("cxx_builtin_include_directories = ", ctx.attr.cxx_builtin_include_directories)
    print("toolchain_identifier = " + ctx.attr.cpu + "-toolchain")
    print("host_system_name = " + "x86_64-linux-gnu")
    print("target_system_name = " + target_system_name[ctx.attr.cpu])
    print("target_cpu = " + ctx.attr.cpu)
    print("target_libc = " + ctx.attr.target_libc)
    print("compiler = " + ctx.attr.compiler)
    print("abi_version = " + ctx.attr.abi_version)
    print("abi_libc_version = " + ctx.attr.abi_libc_version)
    print("tool_paths = ", tool_paths)
    print("make_variables = ", ctx.attr.make_variables)

    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        features = features,
        action_configs = [],
        artifact_name_patterns = [],
        cxx_builtin_include_directories = ctx.attr.cxx_builtin_include_directories,
        toolchain_identifier = ctx.attr.cpu + "-toolchain",
        host_system_name = "x86_64-linux-gnu",
        target_system_name = target_system_name[ctx.attr.cpu],
        target_cpu = ctx.attr.cpu,
        target_libc = ctx.attr.target_libc,
        compiler = ctx.attr.compiler,
        abi_version = ctx.attr.abi_version,
        abi_libc_version = ctx.attr.abi_libc_version,
        tool_paths = tool_paths,
        make_variables = ctx.attr.make_variables,
    )

cc_toolchain_config = rule(
    implementation = _impl,
    attrs = {
        "abi_libc_version": attr.string(mandatory = True, default = "musleabi"),
        "abi_version": attr.string(mandatory = True, default = "eabi"),
        "c_version": attr.string(mandatory = True, default = "c99"),
        "compile_flags": attr.string_list(),
        "compiler": attr.string(mandatory = True, default = "gcc"),
        "cpp_version": attr.string(default = "c++2a"),
        "cpu": attr.string(mandatory = True, values = ["armv7a", "armv6", "aarch64", "k8", "riscv64"]),
        "cxx_builtin_include_directories": attr.string_list(),
        "dbg_compile_flags": attr.string_list(),
        "link_flags": attr.string_list(),
        "link_libs": attr.string_list(),
        "make_variables": attr.string_list(),
        "opt_compile_flags": attr.string_list(),
        "opt_link_flags": attr.string_list(),
        "target_libc": attr.string(mandatory = True),
        "tool_paths": attr.label(mandatory = True, allow_files = True),
        "unfiltered_compile_flags": attr.string_list(),
    },
    provides = [CcToolchainConfigInfo],
)
