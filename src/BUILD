cc_binary(
    name = "wsfs",
    srcs = glob(["*.h", "*.c"]),
    conlyopts = ["-std=c11"],
    copts = ["-pthread -DLOG_LEVEL=2"],
    linkopts = ["-pthread"],
    data = ["staticfiles"],
)
