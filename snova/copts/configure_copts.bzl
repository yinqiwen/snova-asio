SNOVA_DEFAULT_COPTS = select({
    "@bazel_tools//src/conditions:windows": [
        # "/std:c++20",
        "/O2",
    ],
    "//conditions:default": [
        # "-std=c++20",
        "-Werror=return-type",
        # "-fcoroutines",
        # "-DASIO_HAS_IO_URING",
        # "-DASIO_DISABLE_EPOLL",
        "-O2",
        "-g",
    ],
})

SNOVA_DEFAULT_LINKOPTS = [
    # "-lpthread",
    # "-static-libstdc++",
    # "-static-libgcc",
    # "-l:libstdc++.a",
]
