#include "include/entities.h"
#include "../my_lib/stack.h"
#include "../engine_core/include/engine_core.h"

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

void teardown_entity_system(void)
{
	free_stack(&available_entity_ids);
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


bool out_of_bounds(entity_id id, float32 left_lim, float32 right_lim,
                          float32 top_lim, float32 bottom_lim)
{
    position_component top_left = positions[id.index];
    position_component bottom_right;
    bottom_right.x = positions[id.index].x + bounding_boxes[id.index].width - 1;
    bottom_right.y = positions[id.index].y + bounding_boxes[id.index].height- 1;

    // check out of bounds with the top
    if(top_left.y < top_lim) {
        return true;
    }

    // check out of bounds with the bottom
    if(bottom_right.y >= bottom_lim) {
        return true;
    }

    // check out of bounds with left side
    if(top_left.x < left_lim) {
        return true;
    }

    // NOTE: >= because of how arrays work
    // not sure if this logic should be here or when calling

    // check out of bounds with right side
    if(bottom_right.x >= right_lim) {
        return true;
    }

    return false;
}

bool collide(entity_id obj1, entity_id obj2)
{
    uint64 key = BBOX_COMP | POSITION_COMP;
    ASSERT((components_used[obj1.index] & key) == key,
           "Trying to perform collisions on an object that has no bounding box "
           "or position! (first object in collision - entity_id: %ld)\n",
           obj1);
    ASSERT((components_used[obj2.index] & key) == key,
           "Trying to perform collisions on an object that has no bounding box "
           "or position! (second object in collision - entity_id: %ld)\n",
           obj2);

    bounding_box_component bbox1 = bounding_boxes[obj1.index];
    bounding_box_component bbox2 = bounding_boxes[obj2.index];

    position_component pos1 = positions[obj1.index];
    position_component pos2 = positions[obj2.index];

    // check if there are collisions on both X and Y axes since we are using
    // AABB
    return (pos1.x + bbox1.width >= pos2.x
            && pos2.x + bbox2.width >= pos1.x
            && pos1.y + bbox1.height >= pos2.y
            && pos2.y + bbox2.height >= pos1.y);
}

void update_entity_positions(float64 delta)
{
    uint64 key = POSITION_COMP | DIRECTION_COMP;

    for(uint64 i = 0; i < MAX_ENTITIES; i++) {
        entity_id id = entity_id_array[i];
        if(entity_in_use[id.index]
           && ((components_used[id.index] & key) == key)) {
            positions[id.index].x += (directions[id.index].x * delta);
            positions[id.index].y += (directions[id.index].y * delta);
        }
    }
}
