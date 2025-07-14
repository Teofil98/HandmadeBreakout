#pragma once

#include "../../my_lib/defines.h"

#define MAX_COMPONENTS 64

#define POSITION_COMP (1 << 0)
#define SPRITE_COMP (1 << 1)
#define BBOX_COMP (1 << 2)
#define DIRECTION_COMP (1 << 3)

typedef struct position_component {
    float32 x;
    float32 y;
}position_component;

typedef struct sprite_component {
    const uint8* sprite;
    uint32 color;
    uint32 height;
    uint32 width;
}sprite_component;

typedef struct bounding_box_component {
    uint32 height;
    uint32 width;
}bounding_box_component;

typedef struct direction_component {
    float32 x;
    float32 y;
}direction_component;

