// clang-format Language: C
#ifndef DEFINES_H
#define DEFINES_H

#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic warning "-Wunused-function"
#pragma GCC diagnostic warning "-Wunused-variable"
#pragma GCC diagnostic warning "-Wunused-but-set-variable"
// #pragma GCC diagnostic warning "-Wpedantic"

#include <stdint.h>

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t uint8;
typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t int8;
typedef float float32;
typedef double float64;
typedef int32 status;

#define UNUSED(x) (void)x
#define RGBA_ALPHA(x) ((uint8)(x) << 24)
#define RGBA_RED(x) ((uint8)(x) << 16)
#define RGBA_GREEN(x) ((uint8)(x) << 8)
#define RGBA_BLUE(x) ((uint8)(x) << 0)
#define RGBA(r, g, b, a)                                                       \
    (RGBA_RED(r) | RGBA_GREEN(g) | RGBA_BLUE(b) | RGBA_ALPHA(a))

#define COLOR_WHITE RGBA(255, 255, 255, 0)
#define COLOR_BLACK RGBA(0, 0, 0, 0)
#define COLOR_GREEN RGBA(0, 255, 0, 0)
#define COLOR_RED RGBA(255, 0, 0, 0)

#define DEFAULT_WINDOW_W 1280
#define DEFAULT_WINDOW_H 720
#define WINDOW_TITLE "Handmade Space Invaders"

#define PI 3.14159265359

#define STATUS_SUCCESS 0
#define STATUS_FAILURE -1

#define UINT32_MAXIMUM 0xFFFFFFFF

#endif // DEFINES_H
