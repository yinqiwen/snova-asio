SNOVA_DEFAULT_COPTS = [
    "-D__STDC_FORMAT_MACROS",
    "-D__STDC_LIMIT_MACROS",
    "-D__STDC_CONSTANT_MACROS",
    "-DGFLAGS_NS=google",
    "-Werror=return-type",
    "-fcoroutines",
    # "-DASIO_HAS_IO_URING",
    # "-DASIO_DISABLE_EPOLL",
    "-O2",
    "-g",
]

SNOVA_DEFAULT_LINKOPTS = [
    # "-lpthread",
    # "-static-libstdc++",
    # "-static-libgcc",
    # "-l:libstdc++.a",
]
