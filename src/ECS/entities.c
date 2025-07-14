#include "include/entities.h"
#include "../my_lib/stack.h"

position_component positions[MAX_ENTITIES];
sprite_component sprites[MAX_ENTITIES];
bounding_box_component bounding_boxes[MAX_ENTITIES];
direction_component directions[MAX_ENTITIES];
entity_id entity_id_array[MAX_ENTITIES];

uint64 components_used[MAX_ENTITIES];
// OPTIMIZE: Can optimize this with bit operations instead of bools
bool entity_in_use[MAX_ENTITIES];
//static stack<entity_id, MAX_ENTITIES> available_entity_ids;
static stack available_entity_ids;

void init_entity_system(void)
{
    init_stack(&available_entity_ids, MAX_ENTITIES);
    for(uint64 i = 0; i < MAX_ENTITIES; i++) {
        entity_id id;
        id.index = i;
        id.version = 0;
        entity_id_array[i] = id;
        entity_id* new_id = (entity_id*)malloc(sizeof(entity_id));
        *new_id = id;
        stack_push(&available_entity_ids, (void*)new_id);
        // TODO: Technically don't need this since it's gloabal and will be
        // automatically initialized to 0. But I like to be explicit :)
        entity_in_use[id.index] = false;
    }
}

entity_id get_new_entity_id(void)
{
    entity_id* old_id = (entity_id*)stack_pop(&available_entity_ids);
    entity_id id = *old_id;
    entity_in_use[id.index] = true;
    free(old_id);
    return id;
}

void delete_entity_id(entity_id id)
{
    entity_in_use[id.index] = false;
    components_used[id.index] = 0;
    id.version++;
    entity_id_array[id.index].version++;
    entity_id* new_id = (entity_id*)malloc(sizeof(entity_id));
    *new_id = id;
    stack_push(&available_entity_ids, (void*)new_id);
}

bool entity_valid(entity_id id)
{
    return id.version == entity_id_array[id.index].version;
}
