load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_file")

def clean_dep(dep):
    return str(Label(dep))

def snova_workspace(path_prefix = "", tf_repo_name = "", **kwargs):
    http_archive(
        name = "bazel_skylib",
        urls = [
            "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
            "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
        ],
        sha256 = "1c531376ac7e5a180e0237938a2536de0c54d93f5c278634818e0efc952dd56c",
    )
    http_archive(
        name = "rules_foreign_cc",
        strip_prefix = "rules_foreign_cc-0.8.0",
        url = "https://github.com/bazelbuild/rules_foreign_cc/archive/0.8.0.zip",
    )

    git_repository(
        name = "rules_cc",
        commit = "daf6ace7cfeacd6a83e9ff2ed659f416537b6c74",
        remote = "https://github.com/bazelbuild/rules_cc.git",
    )

    _ASIO_BUILD_FILE = """
cc_library(
    name = "asio",
    hdrs = glob([
        "**/*.ipp",
        "**/*.hpp",
    ]),
    includes=["./asio/include"],
    visibility = [ "//visibility:public" ],
)
"""
    asio_ver = kwargs.get("asio_ver", "1-22-1")
    asio_name = "asio-asio-{ver}".format(ver = asio_ver)
    http_archive(
        name = "asio",
        strip_prefix = asio_name,
        build_file_content = _ASIO_BUILD_FILE,
        urls = [
            "https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-{ver}.tar.gz".format(ver = asio_ver),
        ],
    )

    abseil_ver = kwargs.get("abseil_ver", "20211102.0")
    abseil_name = "abseil-cpp-{ver}".format(ver = abseil_ver)
    http_archive(
        name = "com_google_absl",
        strip_prefix = abseil_name,
        urls = [
            "https://github.com/abseil/abseil-cpp/archive/refs/tags/{ver}.tar.gz".format(ver = abseil_ver),
        ],
    )

    spdlog_ver = kwargs.get("spdlog_ver", "1.10.0")
    spdlog_name = "spdlog-{ver}".format(ver = spdlog_ver)
    http_archive(
        name = "com_github_spdlog",
        strip_prefix = spdlog_name,
        urls = [
            "https://github.com/gabime/spdlog/archive/v{ver}.tar.gz".format(ver = spdlog_ver),
        ],
        build_file = clean_dep("//:bazel/spdlog.BUILD"),
    )

    mbedtls_ver = kwargs.get("mbedtls_ver", "3.1.0")
    mbedtls_name = "mbedtls-{ver}".format(ver = mbedtls_ver)
    http_archive(
        name = "com_github_mbedtls",
        strip_prefix = mbedtls_name,
        urls = [
            "https://github.com/Mbed-TLS/mbedtls/archive/v{ver}.tar.gz".format(ver = mbedtls_ver),
        ],
        build_file = clean_dep("//:bazel/mbedtls.BUILD"),
    )

    gtest_ver = kwargs.get("gtest_ver", "1.11.0")
    gtest_name = "googletest-release-{ver}".format(ver = gtest_ver)
    http_archive(
        name = "com_google_googletest",
        strip_prefix = gtest_name,
        urls = [
            "https://github.com/google/googletest/archive/release-{ver}.tar.gz".format(ver = gtest_ver),
        ],
    )

    jemalloc_ver = kwargs.get("jemalloc_ver", "5.3.0")
    jemalloc_name = "jemalloc-{ver}".format(ver = jemalloc_ver)
    http_archive(
        name = "com_github_jemalloc",
        strip_prefix = jemalloc_name,
        urls = [
            "https://github.com/jemalloc/jemalloc/releases/download/{ver}/jemalloc-{ver}.tar.bz2".format(ver = jemalloc_ver),
        ],
        build_file = clean_dep("//:bazel/jemalloc.BUILD"),
    )

    mimalloc_ver = kwargs.get("mimalloc_ver", "2.0.6")
    mimalloc_name = "mimalloc-{ver}".format(ver = mimalloc_ver)
    http_archive(
        name = "com_github_microsoft_mimalloc",
        strip_prefix = mimalloc_name,
        build_file = clean_dep("//:bazel/mimalloc.BUILD"),
        urls = [
            "https://github.com/microsoft/mimalloc/archive/v{ver}.tar.gz".format(ver = mimalloc_ver),
        ],
    )

    _CLI11_BUILD_FILE = """
cc_library(
    name = "CLI11",
    hdrs = glob([
        "include/**",
    ]),
    includes=["./include"],
    visibility = [ "//visibility:public" ],
)
"""
    http_archive(
        name = "com_github_CLIUtils_CLI11",
        strip_prefix = "CLI11-2.2.0",
        build_file_content = _CLI11_BUILD_FILE,
        urls = [
            "https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.2.0.tar.gz",
        ],
    )

    # native.new_local_repository(
    #     name = "armv5l_linux_musleabi",
    #     path = "/opt/musl_gcc_compiles/armv5l-linux-musleabi-cross",
    #     build_file = clean_dep("//:toolchains/armv5l_linux_musleabi.BUILD"),
    # )

    # native.new_local_repository(
    #     name = "armv7l_linux_musleabihf",
    #     path = "/opt/musl_gcc_compiles/armv7l-linux-musleabihf-cross",
    #     build_file = clean_dep("//:toolchains/armv7l_linux_musleabihf.BUILD"),
    # )

    # native.new_local_repository(
    #     name = "x64_linux_musl_gcc",
    #     path = "/opt/musl_gcc_compiles/x86_64-linux-musl-native",
    #     build_file = clean_dep("//:toolchains/x86_64-linux-musl.BUILD"),
    # )

    http_archive(
        name = "armv5l_linux_musleabi",
        strip_prefix = "armv5l-linux-musleabi-cross",
        build_file = clean_dep("//:toolchains/armv5l_linux_musleabi.BUILD"),
        urls = [
            "https://more.musl.cc/11/x86_64-linux-musl/armv5l-linux-musleabi-cross.tgz",
        ],
    )

    http_archive(
        name = "armv7l_linux_musleabihf",
        strip_prefix = "armv7l-linux-musleabihf-cross",
        build_file = clean_dep("//:toolchains/x86_64-linux-musl.BUILD"),
        urls = [
            "https://more.musl.cc/11/x86_64-linux-musl/armv7l-linux-musleabihf-cross.tgz",
        ],
    )

    http_archive(
        name = "x64_linux_musl_gcc",
        strip_prefix = "x86_64-linux-musl-native",
        build_file = clean_dep("//:toolchains/x86_64-linux-musl.BUILD"),
        urls = [
            "https://more.musl.cc/11/x86_64-linux-musl/x86_64-linux-musl-native.tgz",
        ],
        patch_cmds = ["rm -if usr", "mkdir usr", "cp -R include usr"],
    )
