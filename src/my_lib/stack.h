// clang-format Language: C
#pragma once

// TODO: Implement with macros

#include "defines.h"
#include "logging.h"
#include <stdio.h>

typedef struct stack
{
    void** data;
    uint64 top;
    uint64 size;
} stack;

static inline void init_stack(stack* s, uint64 size)
{
    s->top = 0;
    s->size = size;
    s->data = (void**)malloc(sizeof(void*) * size);
}

static inline void free_stack(stack* s)
{
    for(uint64 i = 0; i < s->top; i++) {
        free(s->data[i]);
    }
    free((void*)s->data);
}

static inline void stack_push(stack* s, void* val)
{
    ASSERT(s->top < s->size, "Pushing on full stack! Stack overflow!");
    s->data[s->top] = val;
    s->top++;
}

static inline void* stack_pop(stack* s)
{
    ASSERT(s->top > 0, "Trying to pop empty stack!");
    void* val = s->data[s->top - 1];
    s->top--;
    return val;
}
