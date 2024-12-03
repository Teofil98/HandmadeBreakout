#pragma once

#include "defines.h"

#define MAX_COMPONENTS 64

#define POSITION_COMP (1 << 0)
#define SPRITE_COMP (1 << 1)
#define BBOX_COMP (1 << 2)
#define DIRECTION_COMP (1 << 3)

struct position_component {
    float32 x;
    float32 y;
};

struct sprite_component {
    const uint8* sprite;
    uint32 color;
    uint32 height;
    uint32 width;
};

struct bounding_box_component {
    uint32 height;
    uint32 width;
};

struct direction_component {
    float32 x;
    float32 y;
};

