#pragma once

namespace my_lib {

template <typename T>
inline T max(T a, T b)
{
    return ((a > b) ? a : b);
}

} // namespace my_lib
