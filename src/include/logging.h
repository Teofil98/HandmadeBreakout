#pragma once

#include "config.h"
#include "defines.h"
#include <stdarg.h> // TODO: Delete once testing done
#include <stdio.h>  // TODO: Delete later
#include <stdlib.h> // TODO: Delete later

static void _vlog_print(const char* error_type, const char* filename,
                        const int32 line, const char* frmat, va_list args)
{
    printf("[%s:%d] %s: ", filename, line, error_type);
    vprintf(frmat, args);
}

static void _log_print(const char* error_type, const char* filename,
                       const int32 line, const char* frmat, ...)
{
    va_list args;
    va_start(args, frmat);
    _vlog_print(error_type, filename, line, frmat, args);
    va_end(args);
}

static void _log_error(const char* filename, const int32 line,
                       const char* format, ...)
{
    va_list args;
    va_start(args, format);
    _vlog_print("ERROR", filename, line, format, args);
    va_end(args);
    exit(1);
}

static void _assert(const char* filename, const int line, const bool condition,
                    const char* format, ...)
{
    if(!condition) {
        va_list args;
        va_start(args, format);
        _vlog_print("ASSERT_FAIL", filename, line, format, args);
        va_end(args);
        exit(1);
    }
}

#if ENABLE_ERRORS == 1
#define LOG_ERROR(...) _log_error(__FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_ERROR(...)
#endif

#if ENABLE_WARNINGS == 1
#define LOG_WARNING(...) _log_print("WARNING", __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_WARNING(...)
#endif

#if ENABLE_TRACE == 1
#define LOG_TRACE(...) _log_print("TRACE", __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_TRACE(...)
#endif

#if ENABLE_ASSERTS == 1
#define ASSERT(condition, format, ...) _assert(__FILE__, __LINE__, condition, format, __VA_ARGS__)
#else
#define ASSERT(condition, ...)
#endif
