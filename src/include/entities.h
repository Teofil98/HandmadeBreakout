#pragma once

#include "components.h"
#include "defines.h"

#define MAX_ENTITIES 100

using entity_id = uint64;

extern position_component positions[MAX_ENTITIES];
extern sprite_component sprites[MAX_ENTITIES];
extern bounding_box_component bounding_boxes[MAX_ENTITIES];
extern direction_component directions[MAX_ENTITIES];

extern bool entity_in_use[MAX_ENTITIES];
extern uint64 components_used[MAX_ENTITIES];

void init_entity_system();
entity_id get_new_entity_id();
void delete_entity_id(entity_id id);
