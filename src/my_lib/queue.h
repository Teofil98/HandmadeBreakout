#pragma once

#include "defines.h"
#include <stdio.h>

template <typename T, uint64 N> class queue
{
public:
    queue() : front { 0 }, end { 0 } {}

    void enqueue(T val);
    T dequeue();

    void print()
    {
        printf("[");
        for(int i = front; i < end; i++) {
            printf("%ld ", data[i]);
        }
        printf("]\n");
    }

private:
    T data[N];
    uint64 front;
    uint64 end;
};

template <typename T, uint64 N> void queue<T, N>::enqueue(T val)
{
    data[end] = val;
    // wrap around
    if(end == N - 1) {
        end = 0;
    } else {
        end++;
    }
}

template <typename T, uint64 N> T queue<T, N>::dequeue()
{
    int val = data[front];
    // wrap around
    if(front == N - 1) {
        front = 0;
    } else {
        front++;
    }
    return val;
}
