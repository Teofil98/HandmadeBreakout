#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "defines.h"
#include "logging.h"

typedef struct circular_buffer {
    void** buffer;
    uint64 head;
    uint64 tail;
    uint64 size;
} circular_buffer;

static inline uint64 circular_buffer_inc_pointer(circular_buffer* c_buf, uint64 ptr)
{
    ptr++;
    if(ptr == c_buf->size) {
        ptr = 0;
    }
    return ptr;
}

static inline bool circular_buffer_is_empty(circular_buffer* c_buf)
{
    return c_buf->head == c_buf->tail;
}

static inline bool circular_buffer_is_full(circular_buffer* c_buf)
{
    return circular_buffer_inc_pointer(c_buf, c_buf->head) == c_buf->tail;
}

static inline void init_circular_buffer(circular_buffer* c_buf, uint64 size)
{
    c_buf->buffer = (void**)malloc(size * sizeof(void*));
    c_buf->head = 0;
    c_buf->tail = 0;
    c_buf->size = size;
}

static inline void circular_buffer_insert(circular_buffer* c_buf, void* value)
{
    ASSERT(!circular_buffer_is_full(c_buf),
           "[insert] Circular buffer is full!");
    c_buf->buffer[c_buf->head] = value;
    c_buf->head = circular_buffer_inc_pointer(c_buf, c_buf->head);
}

static inline void* circular_buffer_read(circular_buffer* c_buf)
{
    ASSERT(!circular_buffer_is_empty(c_buf), "[read] Circular buffer is empty");
    void* value = c_buf->buffer[c_buf->tail];
    c_buf->tail = circular_buffer_inc_pointer(c_buf, c_buf->tail);
    return value;
}

static inline void free_circular_buffer(circular_buffer* c_buf)
{
    while(!circular_buffer_is_empty(c_buf)) {
        void* ptr = circular_buffer_read(c_buf);
        free(ptr);
    }
    free((void*)(c_buf->buffer));
}

