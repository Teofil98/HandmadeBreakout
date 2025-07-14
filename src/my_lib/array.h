// clang-format Language: C
#pragma once

#include "defines.h"
#include "logging.h"

// TODO: Implement with macros
typedef struct array {
    void** elements;
    uint64 size;
    uint64 capacity;
} array;

static inline void array_add(array* a, void* val)
{
    ASSERT(size < N, "Array full!\n");
    a->elements[a->size] = val;
    (a->size)++;
}

// Fast delete, changes the relative order of the elements
static inline void array_delete_idx_fast(array* a, uint64 idx)
{
    ASSERT(idx < size, "Array buffer overflow!\n");
    // ASSERT(idx >= 0, "Trying to delete negative index!\n");
    free(a->elements[idx]);
    a->elements[idx] = a->elements[a->size - 1];
    (a->size)--;
}

static inline void* array_get(array* a, uint64 idx) { return a->elements[idx]; }

static inline void array_set(array* a, uint64 idx, void* val)
{
    a->elements[idx] = val;
}

static inline void array_resize(array* a, uint64 new_size)
{
    ASSERT(new_size <= N, "Trying to resize outside of array bounds\n");
    a->size = new_size;
}

static inline uint64 array_get_size(array* a) { return a->size; }

static inline uint64 array_get_capacity(array* a) { return a->capacity; }

static inline void init_array(array* a, uint64 capacity)
{
    a->elements = (void**)malloc(capacity * sizeof(void*));
    a->capacity = capacity;
    a->size = 0;
}

static inline void free_array(array* a)
{
    for(uint64 i = 0; i < a->size; i++) {
        free(a->elements[i]);
    }
    free((void*)a->elements);
}
