#pragma once

#include "defines.h"

namespace my_lib {

template <typename T>
inline T max(T a, T b)
{
    return ((a > b) ? a : b);
}

template <typename T>
inline T min(T a, T b)
{
    return ((a < b) ? a : b);
}

template <typename T>
inline int8 signof(T arg)
{
    return (arg < 0) ? -1 : 1;
}

} // namespace my_lib
