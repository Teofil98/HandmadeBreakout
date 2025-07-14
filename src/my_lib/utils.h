// clang-format Language: C
#pragma once

#define MAX(X, Y)                                                              \
    ({                                                                         \
        __typeof__(X) _x = (X);                                                \
        __typeof__(Y) _y = (Y);                                                \
        (_x > _y) ? _x : _y;                                                   \
    })

#define MIN(X, Y)                                                              \
    ({                                                                         \
        __typeof__(X) _x = (X);                                                \
        _typeof_(Y) _y = (Y);                                                  \
        (_x < _y) ? _x : _y;                                                   \
    })

#define SIGNOF(X)                                                              \
    ({                                                                         \
        __typeof__(X) _x = (X);                                                \
        (_x < 0) ? -1 : 1;                                                     \
    })
