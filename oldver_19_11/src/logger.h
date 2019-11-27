#include <stdio.h>
#include <stdlib.h>

#if !defined(LOGGER_H)
#define LOGGER_H

const int LOG_LV_DEBUG = 1;
const int LOG_LV_VERBOSE = 2;
const char LOG_LV_STR[][20] = {"", "DEBUG", "VERBOSE"};

int LOG_LEVEL = LOG_LV_VERBOSE;

FILE *FP_LOG_OUTPUT = stderr;

// 设置日志等级
#define LOGGER_SET_LV(x)                              \
    do {                                              \
        if (x >= LOG_LV_DEBUG && x <= LOG_LV_VERBOSE) \
            LOG_LEVEL = x;                            \
    } while (0)

#define LOGGER_SIMP(level, str)                                                                                   \
    do {                                                                                                          \
        if (level <= LOG_LEVEL && level > 0)                                                                      \
            fprintf(FP_LOG_OUTPUT, "LOG %s:--- %s --- LOG %s OVER\n", LOG_LV_STR[level], str, LOG_LV_STR[level]); \
    } while (0)

#define LOGGER_FORMAT(level, format, ...)                                       \
    do {                                                                        \
        if (level <= LOG_LEVEL && level > 0) {                                  \
            fprintf(FP_LOG_OUTPUT, "LOG %s:--- ", LOG_LV_STR[level]);           \
            fprintf(FP_LOG_OUTPUT, format, __VA_ARGS__);                        \
            fprintf(FP_LOG_OUTPUT, ". ---LOG %s OVER\n", LOG_LV_STR[level]); \
        }                                                                       \
    } while (0)

#endif // LOGGER_H