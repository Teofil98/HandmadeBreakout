#include "defines.h"

class random_number_generator
{
public:
    uint32 state = 0;

    void init_seed32(uint32 seed);
    uint32 rand_int32();
    // get a random uint32 in the interval [start, end]
    uint32 rand_int32(uint32 start, uint32 end);

private:
    void xorshift32(void);
};

/* The state must be initialized to non-zero */
inline void random_number_generator::xorshift32(void)
{
    /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
}

inline uint32 random_number_generator::rand_int32()
{
    xorshift32();
    return state;
}

inline uint32 random_number_generator::rand_int32(uint32 start, uint32 end)
{
    xorshift32();
    // NOTE: More robust but slower alternative
    //float64 ratio = (float64)state / UINT32_MAXIMUM;
    //return (uint32)((end - start) * ratio + start);
    return state % (end + 1 - start) + start;
}

inline void random_number_generator::init_seed32(uint32 seed)
{
    state = seed;
}
