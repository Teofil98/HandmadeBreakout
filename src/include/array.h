#pragma once

#include "defines.h"
#include "logging.h"

namespace my_lib {

template <typename T, uint64 N>
class array
{
public:

    array() : size{0} {};

    void add(T val) {
        ASSERT(size < N, "Array full!\n");
        elements[size] = val;
        size++;
    }

    // Fast delete, changes the relative order of the elements
    void delete_idx_fast(uint64 idx)
    {
        ASSERT(idx < size, "Array buffer overflow!\n");
        //ASSERT(idx >= 0, "Trying to delete negative index!\n");
        elements[idx] = elements[size - 1];
        size--;
    }

    // TODO: Test the operators
    T& operator[](uint64 idx)
    {
        ASSERT(idx < size, "Array buffer overflow! (idx: %ld, size: %ld)\n");
        return elements[idx];
    }

    T operator[](uint64 idx) const
    {
        const T elem = this[idx];
        return elem;
    }

    void resize(uint64 new_size)
    {
        ASSERT(new_size <= N, "Trying to resize outside of array bounds\n");
        size = new_size;
    }

    uint64 get_size()
    {
        return size;
    }

private:
    T elements[N];
    uint64 size;
};

} // namespace my_lib

