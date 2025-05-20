#pragma once

#include "defines.h"
#include "logging.h"

template <typename T, uint64 N>
class circular_buffer
{
public:
    circular_buffer() : head { 0 }, tail { 0 } {};
    void insert(T value);
    T read(void);
    bool is_full(void);
    bool is_empty(void);
private:
    T buffer[N];
    uint64 head;
    uint64 tail;
    uint64 increment_pointer(uint64 ptr);
};


template <typename T, uint64 N>
void circular_buffer<T,N>::insert(T value)
{
    ASSERT(!is_full(), "[insert] Circular buffer is full!");
    buffer[head] = value;
    head = increment_pointer(head);
}

template <typename T, uint64 N>
T circular_buffer<T,N>::read(void)
{
    ASSERT(!is_empty(), "[read] Circular buffer is empty");
    T value = buffer[tail];
    tail = increment_pointer(tail);
    return value;
}

template <typename T, uint64 N>
bool circular_buffer<T,N>::is_full(void)
{
    return increment_pointer(head) == tail;
}

template <typename T, uint64 N>
bool circular_buffer<T,N>::is_empty(void)
{
    return head == tail;
}

template <typename T, uint64 N>
uint64 circular_buffer<T,N>::increment_pointer(uint64 ptr)
{
    ptr++;
    if(ptr == N) {
        ptr = 0;
    }
    return ptr;
}
