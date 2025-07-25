#pragma once

#include "components.h"
#include "../../my_lib/defines.h"
#include <stdbool.h>

#define MAX_ENTITIES 100

typedef struct entity_id {
    uint64 index : 32;
    uint64 version : 32;
} entity_id;

extern entity_id entity_id_array[MAX_ENTITIES];

extern position_component positions[MAX_ENTITIES];
extern sprite_component sprites[MAX_ENTITIES];
extern bounding_box_component bounding_boxes[MAX_ENTITIES];
extern direction_component directions[MAX_ENTITIES];

extern bool entity_in_use[MAX_ENTITIES];
extern uint64 components_used[MAX_ENTITIES];

void init_entity_system(void);
void teardown_entity_system(void);
entity_id get_new_entity_id(void);
void delete_entity_id(entity_id id);
bool entity_valid(entity_id id);
