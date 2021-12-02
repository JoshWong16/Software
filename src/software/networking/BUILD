package(default_visibility = ["//visibility:public"])

cc_library(
    name = "proto_udp_client",
    hdrs = [
        "proto_udp_client.hpp",
    ],
    deps = [
        "//software/logger",
        "//software/util/typename",
    ],
)

cc_library(
    name = "proto_udp_listener",
    hdrs = [
        "proto_udp_listener.hpp",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//software/logger",
        "//software/util/typename",
    ],
)

cc_library(
    name = "proto_unix_listener",
    hdrs = [
        "proto_unix_listener.hpp",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//software/logger",
        "//software/util/typename",
    ],
)

cc_library(
    name = "proto_udp_sender",
    hdrs = [
        "proto_udp_sender.hpp",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//software/logger",
        "@boost//:asio",
    ],
)

cc_library(
    name = "threaded_proto_udp_listener",
    srcs = [
        "threaded_proto_udp_listener.hpp",
    ],
    deps = [
        ":proto_udp_listener",
        "@boost//:asio",
    ],
)

cc_library(
    name = "threaded_proto_unix_listener",
    srcs = [
        "threaded_proto_unix_listener.hpp",
    ],
    deps = [
        ":proto_unix_listener",
        "@boost//:asio",
    ],
)

cc_library(
    name = "threaded_proto_udp_sender",
    hdrs = [
        "threaded_proto_udp_sender.hpp",
    ],
    deps = [
        ":proto_udp_sender",
        "@boost//:asio",
    ],
)

cc_library(
    name = "proto_unix_sender",
    hdrs = [
        "proto_unix_sender.hpp",
    ],
    visibility = ["//visibility:private"],
    deps = [
        "//software/logger",
        "@boost//:asio",
    ],
)

cc_library(
    name = "threaded_proto_unix_sender",
    hdrs = [
        "threaded_proto_unix_sender.hpp",
    ],
    deps = [
        ":proto_unix_sender",
        "@boost//:asio",
    ],
)

