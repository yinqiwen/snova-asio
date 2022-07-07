SNOVA_DEFAULT_COPTS = select({
    "@bazel_tools//src/conditions:windows": [
        # "/std:c++20",
        "/O2",
        "/W3",
        "/DNOMINMAX",
        "/DWIN32_LEAN_AND_MEAN",
        "/D_CRT_SECURE_NO_WARNINGS",
        "/D_SCL_SECURE_NO_WARNINGS",
        "/D_ENABLE_EXTENDED_ALIGNED_STORAGE",
        "/bigobj",
        "/wd4005",
        "/wd4068",
        "/wd4180",
        "/wd4244",
        "/wd4267",
        "/wd4503",
        "/wd4800",
        # used for nanopb
        "/DPB_C99_STATIC_ASSERT",
        "/Gy",
    ],
    "//conditions:default": [
        # "-std=c++20",
        "-Werror=return-type",
        # "-fcoroutines",
        # "-DASIO_HAS_IO_URING",
        # "-DASIO_DISABLE_EPOLL",
        "-O3",
        "-fvisibility=hidden",
        "-ffunction-sections",
        "-fdata-sections",
        # "-g",
    ],
})

SNOVA_DEFAULT_LINKOPTS = select({
    "@bazel_tools//src/conditions:windows": [
        "/OPT:REF",
    ],
    "@bazel_tools//src/conditions:darwin": ["-dead_strip"],
    "//conditions:default": [
        "-Wl,--gc-sections,-s",
    ],
})
