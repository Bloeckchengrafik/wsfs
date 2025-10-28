#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define LOG_LEVEL_ERROR 1
#define LOG_LEVEL_INFO 2
#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_TRACE 4

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

#if LOG_LEVEL >= LOG_LEVEL_ERROR
#define WSFS_LOGE(fmt, ...) fprintf(stderr, "[ERROR %10s %3d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define WSFS_LOGE(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
#define WSFS_LOGI(fmt, ...) fprintf(stdout, "[INFO  %10s %3d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define WSFS_LOGI(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
#define WSFS_LOGD(fmt, ...) fprintf(stdout, "[DEBUG %10s %3d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define WSFS_LOGD(fmt, ...) ((void)0)
#endif

#if LOG_LEVEL >= LOG_LEVEL_TRACE
#define WSFS_LOGT(fmt, ...) fprintf(stdout, "[TRACE %10s %3d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define WSFS_LOGT(fmt, ...) ((void)0)
#endif

#define WSFS_CHECKERR(err, fmt, ...) do { \
    if (err) { \
        perror("Reported Error"); \
        WSFS_LOGE(fmt, ##__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } \
} while (0)
