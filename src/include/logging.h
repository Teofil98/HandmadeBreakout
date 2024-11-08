#pragma once

#include "config.h"
#include <stdarg.h> // TODO: Delete once testing done
#include <stdio.h> // TODO: Delete later

static void _log_print(const char* error_type, const char* filename, int line,
                       const char* format, ...)
{
    va_list args;
    va_start(args, format);
    printf("[%s:%d] %s ", filename, line, error_type);
    vprintf(format, args);
    va_end(args);
}

#if ENABLE_ERRORS == 1
#define LOG_ERROR(...) _log_print("ERROR", __FILE__, __LINE__, __VA_ARGS__)
#else
#define LOG_ERROR(...) ((void)0)
#endif
