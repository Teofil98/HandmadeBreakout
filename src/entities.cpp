#include "include/entities.h"
#include "include/stack.h"

position_component positions[MAX_ENTITIES];
sprite_component sprites[MAX_ENTITIES];
bounding_box_component bounding_boxes[MAX_ENTITIES];
direction_component directions[MAX_ENTITIES];

uint64 components_used[MAX_ENTITIES];
// OPTIMIZE: Can optimize this with bit operations instead of bools
bool entity_in_use[MAX_ENTITIES];
static stack<entity_id, MAX_ENTITIES> available_entity_ids;

void init_entity_system()
{
    for(entity_id i = MAX_ENTITIES - 1; i > 0; i--) {
        available_entity_ids.push(i);
        // TODO: Technically don't need this since it's gloabal and will be
        // automatically initialized to 0. But I like to be explicit :)
        entity_in_use[i] = false;
    }
    available_entity_ids.push(0);
    entity_in_use[0] = false;
}

entity_id get_new_entity_id()
{
    entity_id id = available_entity_ids.pop();
    entity_in_use[id] = true;
    return id;
}

void delete_entity_id(entity_id id)
{
    entity_in_use[id] = false;
    components_used[id] = 0;
    available_entity_ids.push(id);
}
