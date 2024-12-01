#pragma once

#include "defines.h"

#define MAX_COMPONENTS 10

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
