// clang-format Language: C
#ifndef RAND_H
#define RAND_H

#include "defines.h"

typedef struct random_number_generator {
    uint32 state;
} random_number_generator;

// The state must be initialized to non-zero
static inline void rand32_xorshift(random_number_generator* rng)
{
    // Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
    rng->state ^= rng->state << 13;
    rng->state ^= rng->state >> 17;
    rng->state ^= rng->state << 5;
}

static inline uint32 rand32_rand(random_number_generator* rng)
{
    rand32_xorshift(rng);
    return rng->state;
}

// get a random uint32 in the interval [start, end]
static inline uint32 rand32_rand_interval(random_number_generator* rng,
                                          uint32 start, uint32 end)
{
    rand32_xorshift(rng);
    // NOTE: More robust but slower alternative
    // float64 ratio = (float64)state / UINT32_MAXIMUM;
    // return (uint32)((end - start) * ratio + start);
    return rng->state % (end + 1 - start) + start;
}

static inline void rand32_init(random_number_generator* rng, uint32 seed)
{
    rng->state = seed;
}

#endif // RAND_H
