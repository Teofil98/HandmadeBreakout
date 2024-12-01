#pragma once

#include "defines.h"
#include "logging.h"
#include <stdio.h>

template <typename T, uint64 N> class stack
{
public:
    stack() : top { 0 } {}

    void push(T val);
    T pop();

    void print()
    {
        printf("[");
        for(int i = 0; i < top; i++) {
            printf("%ld ", data[i]);
        }
        printf("]\n");
    }

private:
    T data[N];
    uint64 top;
};

template <typename T, uint64 N> void stack<T, N>::push(T val)
{
    ASSERT(top < N, "Pushing on full stack! Stack overflow!");
    data[top] = val;
    top++;
}

template <typename T, uint64 N> T stack<T, N>::pop()
{
    ASSERT(top > 0, "Trying to pop empty stack!");
    int val = data[top - 1];
    top--;
    return val;
}
