package(default_visibility = ["//visibility:public"])

MIMALLOC_COPTS = select({
    "@bazel_tools//src/conditions:windows": [
        "/O2 /Zc:__cplusplus /DMI_MALLOC_OVERRIDE /DDMI_STATIC_LIB",
    ],
    "//conditions:default": [
        "-O3",
        "-std=gnu11",
        "-fPIC",
        "-Wall -Wextra -Wno-unknown-pragmas",
        "-fvisibility=hidden",
        "-Wstrict-prototypes",
        "-ftls-model=initial-exec",
        "-fno-builtin-malloc",
        "-DMI_MALLOC_OVERRIDE",
        "-DMI_STATIC_LIB",
        "-DNDEBUG",
    ],
}) + select({
    "@bazel_tools//src/conditions:darwin": [
        "-DMI_OSX_ZONE=1 -DMI_OSX_INTERPOSE=1",
        "-Wpedantic -Wno-static-in-inline",
    ],
    "//conditions:default": [
    ],
})

cc_library(
    name = "mimalloc_headers",
    hdrs = glob([
        "include/*.h",
    ]),
    strip_include_prefix = "include",
)

cc_library(
    name = "libmimalloc",
    srcs = [
        "src/static.c",
    ],
    hdrs = [
        "src/alloc-override.c",
        "src/bitmap.h",
        "src/page-queue.c",
        "src/alloc.c",
        "src/alloc-aligned.c",
        "src/alloc-posix.c",
        "src/arena.c",
        "src/bitmap.c",
        "src/heap.c",
        "src/init.c",
        "src/options.c",
        "src/os.c",
        "src/page.c",
        "src/random.c",
        "src/segment.c",
        "src/segment-cache.c",
        "src/stats.c",
    ] + select({
        "@bazel_tools//src/conditions:darwin": [
            "src/alloc-override-osx.c",
        ],
        "//conditions:default": [
        ],
    }),
    copts = MIMALLOC_COPTS,
    deps = [":mimalloc_headers"],
)
