#pragma once

#include "components.h"
#include "defines.h"

using entity_id = uint64;

extern position_component positions[MAX_ENTITIES];
extern sprite_component sprites[MAX_ENTITIES];
extern bounding_box_component bounding_boxes[MAX_ENTITIES];
extern bool entity_in_use[MAX_ENTITIES];

void init_entity_system();
entity_id get_new_entity_id();
void delete_entity_id(entity_id id);
