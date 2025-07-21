#include "include/game.h"
#include "engine_core/include/engine_core.h"
#include "my_lib/defines.h"
#include "ECS/include/entities.h"
#include "engine_core/include/input.h"
#include "my_lib/logging.h"
#include "engine_core/include/platform_layer.h"
#include "my_lib/array.h"
#include "my_lib/rand.h"
#include "my_lib/utils.h"

// GENERAL TODO: Check all return values of all functions and log errors where
// needed. I Probably missed a few places so far in ALL SOURCE FILES SO FAR :)

#define ALIENS_ROWS 4
#define ALIENS_COLS 6
#define SEED 12345
#define MAX_ALIEN_PROJECTILES 3
#define INITIAL_PROJECTILE_FREQ 2
#define INITIAL_ALIEN_SPEED 15

static screen_information g_screen_info;

static const uint8 spaceship_bytes[] = {
    "-----*-----"
    "----***----"
    "--*-*-*-*--"
    "-**-*-*-**-"
    "***********"
    "***********"
    "-**-***-**-"
    "--*--*--*--"
};

static const uint8 alien_bytes[] = {
    "--*----*--"
    "---*--*---"
    "--******--"
    "-**-**-**-"
    "**********"
    "-********-"
    "--**--**--"
    "--*----*--"
};

static const uint8 projectile_bytes[] = {
    "*"
    "*"
};

// TODO: Refactor in function that initializez to initial state
// and use same function in the reset
static float32 g_spaceship_speed = 30;
static int32 g_projectile_speed = -60;
static int32 g_alien_speed = INITIAL_ALIEN_SPEED;
static int32 g_alien_speed_increment = 15;
static int32 g_dead_aliens = 0;
static entity_id g_spaceship_id;
static array g_aliens;
static array g_aliens_projectiles;
static float64 g_alien_projectile_frequency = INITIAL_PROJECTILE_FREQ;
static float64 g_alien_projectile_frequency_decrement = 0.5;
static bool g_next_alien_collision_side_left = false;
static entity_id g_spaceship_projectile;
static const uint32 g_alien_width = 10;
static const uint32 g_alien_height = 8;
static bool g_player_dead = false;
static platform_window* g_window;
static platform_backbuffer* g_backbuffer;
static platform_sound_buffer* g_sound_buffer_shoot;
static platform_sound_buffer* g_sound_buffer_kill;
static random_number_generator rng;

// TODO: Crearly separate in different layers
// functions that use pixels in screen space
// vs in game space
static entity_id create_spaceship(void)
{
    entity_id id = get_new_entity_id();

    position_component pos;
    pos.x = g_screen_info.width_in_pixels/2.0;
    pos.y = g_screen_info.height_in_pixels - 10;

    sprite_component sprt;
    sprt.color = COLOR_WHITE;
    sprt.sprite = spaceship_bytes;
    sprt.width = 11;
    sprt.height = 8;

    bounding_box_component box;
    box.width = sprt.width;
    box.height = sprt.height;

    positions[id.index] = pos;
    sprites[id.index] = sprt;
    bounding_boxes[id.index] = box;

    components_used[id.index] = POSITION_COMP | SPRITE_COMP | BBOX_COMP;

    return id;
}

static entity_id create_alien(const uint32 row, const uint32 col)
{
    entity_id id = get_new_entity_id();

    position_component pos;
    pos.x = col;
    pos.y = row;

    sprite_component sprt;
    sprt.color = COLOR_WHITE;
    sprt.sprite = alien_bytes;
    sprt.width = g_alien_width;
    sprt.height = g_alien_height;

    // TODO: Refactor components. Add a function to add a new component to an index.
    bounding_box_component box;
    box.width = sprt.width;
    box.height = sprt.height;

    direction_component direction;
    direction.x = g_alien_speed;
    direction.y = 0;

    positions[id.index] = pos;
    sprites[id.index] = sprt;
    bounding_boxes[id.index] = box;
    directions[id.index] = direction;

    components_used[id.index] = POSITION_COMP | SPRITE_COMP | BBOX_COMP | DIRECTION_COMP;

    return id;
}

static void create_alien_matrix(void)
{
    const uint32 initial_col = 3;
    const uint32 initial_row = 3;
    uint32 row_space = 0;
    array_resize(&g_aliens, ALIENS_ROWS * ALIENS_COLS);
    for(uint32 i = 0; i < ALIENS_ROWS; i++) {
        uint32 col_space = 0;
        for(uint32 j = 0; j < ALIENS_COLS; j++) {
            const uint32 row = initial_row + i * g_alien_height + row_space;
            const uint32 col = initial_col + j * g_alien_width + col_space;
            col_space += 3;
            entity_id* elem = (entity_id*)malloc(sizeof(entity_id));
            *elem = create_alien(row, col);
            array_set(&g_aliens, i * ALIENS_COLS + j, (void*)elem) ;
        }
        row_space += 2;
    }
}

static entity_id create_projectile(const uint32 row, const uint32 col,
                                   const float32 dir_x, const float32 dir_y)
{
    entity_id id = get_new_entity_id();

    position_component pos;
    pos.x = col;
    pos.y = row;

    sprite_component sprt;
    sprt.sprite = projectile_bytes;
    sprt.color = COLOR_WHITE;
    sprt.width = 1;
    sprt.height = 2;

    bounding_box_component box;
    box.width = sprt.width;
    box.height = sprt.height;

    direction_component dir;
    dir.x = dir_x;
    dir.y = dir_y;

    positions[id.index] = pos;
    sprites[id.index] = sprt;
    bounding_boxes[id.index] = box;
    directions[id.index] = dir;

    components_used[id.index] = POSITION_COMP | SPRITE_COMP | BBOX_COMP
                          | DIRECTION_COMP;

    return id;
}

static void draw_sprite(const entity_id id, platform_backbuffer* backbuffer)
{
    uint32 height = sprites[id.index].height;
    uint32 width = sprites[id.index].width;
    const uint8* sprite = sprites[id.index].sprite;
    uint32 color = sprites[id.index].color;
    uint32 entity_row = (uint32)positions[id.index].y;
    uint32 entity_col = (uint32)positions[id.index].x;

    for(uint32 i = 0; i < height; i++) {
        for(uint32 j = 0; j < width; j++) {
            if(sprite[width * i + j] == '*') {
                draw_pixel(backbuffer, entity_row + i, entity_col + j, color, &g_screen_info);
            }
        }
    }
}

static void draw_sprites(platform_backbuffer* backbuffer)
{
    for(uint64 i = 0; i < MAX_ENTITIES; i++) {
        entity_id id = entity_id_array[i];
        if(entity_in_use[id.index]) {
            draw_sprite(id, backbuffer);
        }
    }
}

static bool out_of_bounds(entity_id id, float32 left_lim, float32 right_lim,
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

static bool collide(entity_id obj1, entity_id obj2)
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

static void update_alien_speeds(void)
{
    for(uint64 i = 0; i < array_get_size(&g_aliens); i++) {
        // TODO: Fix ugly cast with beautiful macro?
        entity_id alien = *((entity_id*)array_get(&g_aliens,i));
        if(!entity_valid(alien)) {
            continue;
        }

        int8 sign = SIGNOF(directions[alien.index].x);
        directions[alien.index].x = sign * g_alien_speed;
    }
}

static void collide_user_proj(void)
{
    uint64 curr_alien = 0;

    while(curr_alien < array_get_size(&g_aliens)) {
        // TODO: Fix ugly cast with beautiful macro?
        entity_id alien = *((entity_id*)array_get(&g_aliens, curr_alien));
        if(entity_valid(alien) && collide(g_spaceship_projectile, alien)) {
            delete_entity_id(alien);
            delete_entity_id(g_spaceship_projectile);
            play_sound_buffer(g_sound_buffer_kill);
            g_dead_aliens++;
            if(g_dead_aliens == ALIENS_COLS) {
                g_dead_aliens = 0;
                g_alien_speed += g_alien_speed_increment;
                update_alien_speeds();
                g_alien_projectile_frequency
                    -= g_alien_projectile_frequency_decrement;
                g_alien_projectile_frequency = MAX(g_alien_projectile_frequency,
                                                   1.0);
            }
            // TODO: find better solution, eventually the value will overflow
            // back here Don't delete alien, instead, keep it in matrix to mark
            // that it is dead g_aliens.delete_idx_fast(curr_alien);
            break;
        }
        curr_alien++;
    }
}

static void collide_alien_proj(void)
{
    for(uint64 i = 0; i < array_get_size(&g_aliens_projectiles); i++) {
        // TODO: Fix ugly cast with beautiful macro?
        entity_id alien_projectile = *(
            (entity_id*)array_get(&g_aliens_projectiles, i));
        if(entity_valid(alien_projectile)
           && collide(g_spaceship_id, alien_projectile)) {
            g_player_dead = true;
        }
    }
}

static void collide_aliens_with_spaceship(void)
{
    for(uint64 i = 0; i < array_get_size(&g_aliens); i++) {
        // TODO: Fix ugly cast with beautiful macro?
        entity_id alien = *((entity_id*)array_get(&g_aliens, i));
        if(entity_valid(alien) && collide(g_spaceship_id, alien)) {
            g_player_dead = true;
        }
    }
}

static void check_collisions(void)
{
    uint64 key = BBOX_COMP;
    // check out of bounds objects
    for(uint64 i = 0; i < MAX_ENTITIES; i++) {
        entity_id id = entity_id_array[i];
        if(entity_in_use[id.index]
           && ((components_used[id.index] & key) == key)) {
            if(out_of_bounds(id, 0, g_screen_info.width_in_pixels, 0,
                             g_screen_info.height_in_pixels)) {
                delete_entity_id(id);
            }
        }
    }

    if(entity_in_use[g_spaceship_projectile.index]
       && entity_valid(g_spaceship_projectile)) {
        collide_user_proj();
    }

    if(array_get_size(&g_aliens_projectiles) > 0) {
        collide_alien_proj();
    }

    collide_aliens_with_spaceship();
}

static void update_entity_positions(float64 delta)
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


void game_init(const uint32 width_in_pixels, const uint32 height_in_pixels,
               const uint32 pixel_size)
{
    LOG_TRACE("Game init\n");

    const uint32_t bytes_per_pixel = 4;

    init_array(&g_aliens, ALIENS_ROWS * ALIENS_COLS);
    init_array(&g_aliens_projectiles, MAX_ALIEN_PROJECTILES);

    init_screen_info(&g_screen_info, width_in_pixels, height_in_pixels, pixel_size);
    g_window = open_window(WINDOW_TITLE, g_screen_info.screen_width,
                           g_screen_info.screen_height);
    g_backbuffer = create_backbuffer(g_screen_info.screen_width,
                                     g_screen_info.screen_height,
                                     bytes_per_pixel);
    const uint8 channels = 2;
    const uint32 nb_samples_per_sec = 44100;
    const uint8 bits_per_sample = 16;
    init_sound(channels, nb_samples_per_sec, bits_per_sample, WINDOW_TITLE);
    g_sound_buffer_shoot = create_sound_buffer(
        get_frames_from_time_sec(0.2f, nb_samples_per_sec));
    g_sound_buffer_kill = create_sound_buffer(
        get_frames_from_time_sec(0.2f, nb_samples_per_sec));
    init_input();
    init_entity_system();
    rand32_init(&rng, SEED);

    LOG_TRACE("Game init done\n");
}

void game_destroy(void)
{
    LOG_TRACE("Destroying game\n");
    free_array(&g_aliens);
    free_array(&g_aliens_projectiles);
	teardown_entity_system();
    destroy_window(g_window);
    destroy_backbuffer(g_backbuffer);
    destroy_sound_buffer(g_sound_buffer_shoot);
    destroy_sound_buffer(g_sound_buffer_kill);
    teardown_sound();
    teardown_input();
    LOG_TRACE("Destroyed game\n");
}

void process_input(float64 delta)
{
    position_component old_position = positions[g_spaceship_id.index];
    if(keys[KEY_A].held) {
        positions[g_spaceship_id.index].x -= delta * g_spaceship_speed;
    } else if(keys[KEY_D].held) {
        positions[g_spaceship_id.index].x += delta * g_spaceship_speed;
    }

    if(keys[KEY_SPACE].pressed) {
        uint32 proj_row = positions[g_spaceship_id.index].y - 1;
        uint32 proj_col = positions[g_spaceship_id.index].x
                          + sprites[g_spaceship_id.index].width / 2;
        if(!entity_in_use[g_spaceship_projectile.index]
           || !entity_valid(g_spaceship_projectile)) {
            g_spaceship_projectile = create_projectile(proj_row, proj_col, 0,
                                                       g_projectile_speed);
            play_sound_buffer(g_sound_buffer_shoot);
        }
    }

    if(out_of_bounds(g_spaceship_id, 0, g_screen_info.width_in_pixels, 0,
                     g_screen_info.height_in_pixels)) {
        positions[g_spaceship_id.index] = old_position;
    }
}

// OPTIMIZE: Memcpy
static void clear_screen(platform_backbuffer* backbuffer, uint32 color)
{
    uint32* pixels = (uint32_t*)backbuffer->bitmap;
    uint32 num_cols = backbuffer->width;
    for(uint32 i = 0; i < backbuffer->height; i++) {
        for(uint32 j = 0; j < backbuffer->width; j++) {
            pixels[i * num_cols + j] = color;
        }
    }
}

static bool player_won(void)
{
    for(uint64 i = 0; i < array_get_size(&g_aliens); i++) {
        // TODO: Fix ugly cast with beautiful macro?
        entity_id alien = *((entity_id*)array_get(&g_aliens,i));
        if(entity_valid(alien)) {
            // alien still alive
            return false;
        }
    }
    return !g_player_dead;
}

static bool player_lost(void)
{
    return g_player_dead;
}

static entity_id get_alien_to_shoot(void)
{
    // Aliens on the last non-empty row
    // They have a clear line-of-sight to shoot
    entity_id visible_aliens[ALIENS_COLS];
    int32 nb_visible_aliens = 0;
    for(int32 j = 0; j < ALIENS_COLS; j++) {
        for(int32 i = ALIENS_ROWS - 1; i >= 0; i--) {
        // TODO: Fix ugly cast with beautiful macro?
        entity_id alien = *((entity_id*)array_get(&g_aliens, i * ALIENS_COLS + j));
            if(entity_valid(alien)) {
                visible_aliens[nb_visible_aliens] = alien;
                nb_visible_aliens++;
                break;
            }
        }
    }

    if(nb_visible_aliens == 0) {
        // should never happen under current configuration
        // TODO: Put newline automagically in assert macro
        ASSERT(false,
               "Current assumption: If all aliens are dead, game is won");
        entity_id default_res;
        default_res.index = 0;
        default_res.version = 0;
        return default_res;
    }

    int32 idx = rand32_rand_interval(&rng, 0, nb_visible_aliens - 1);
    return visible_aliens[idx];
}

static void generate_alien_projectile(float64 delta,
                                      float64* alien_projectile_timer)
{

    // Remove deleted projectiles
    // NOTE: Really don't like doing it like this
    uint64 i = 0;
    while(i < array_get_size(&g_aliens_projectiles)) {
        // TODO: Fix ugly cast with beautiful macro?
        entity_id alien_projectile = *(
            (entity_id*)array_get(&g_aliens_projectiles, i));
        if(!entity_valid(alien_projectile)) {
            array_delete_idx_fast(&g_aliens_projectiles, i);
            continue;
        }
        i++;
    }

    *alien_projectile_timer -= delta;
    if(*alien_projectile_timer < 0) {
        if(array_get_size(&g_aliens_projectiles)
           < array_get_capacity(&g_aliens_projectiles)) {

            entity_id alien_to_shoot = get_alien_to_shoot();

            uint32 proj_row = positions[alien_to_shoot.index].y + 2;
            uint32 proj_col = positions[alien_to_shoot.index].x
                              + sprites[alien_to_shoot.index].width / 2;

            entity_id* new_projectile = (entity_id*)malloc(sizeof(entity_id));
            *new_projectile = create_projectile(proj_row, proj_col, 0, -g_projectile_speed);
            array_add(&g_aliens_projectiles, (void*)new_projectile);
            play_sound_buffer(g_sound_buffer_shoot);
        }
        *alien_projectile_timer = g_alien_projectile_frequency;
    }
}

static void reset_game_state(float64* delta, float64* curr_time)
{
    g_player_dead = false;
    g_next_alien_collision_side_left = false;
    *delta = 0;
    *curr_time = get_time_ms();
    g_dead_aliens = 0;
    g_alien_speed = INITIAL_ALIEN_SPEED;
    g_alien_projectile_frequency = INITIAL_PROJECTILE_FREQ;
    //delete all entities
    for(uint64 i = 0; i < MAX_ENTITIES; i++) {
        if(entity_in_use[i]) {
            delete_entity_id(entity_id_array[i]);
        }
    }

	// free aliens and projectile arrays and reinitialize them
	
    free_array(&g_aliens);
    free_array(&g_aliens_projectiles);

    init_array(&g_aliens, ALIENS_ROWS * ALIENS_COLS);
    init_array(&g_aliens_projectiles, MAX_ALIEN_PROJECTILES);

    g_spaceship_id = create_spaceship();
    create_alien_matrix();
}

static void update_alien_positions(void)
{
    bool should_update = false;
    for(uint64 i = 0; i < array_get_size(&g_aliens); i++) {
        // TODO: Fix ugly cast with beautiful macro?
        entity_id alien = *((entity_id*)array_get(&g_aliens,i));
        if(!entity_valid(alien)) {
            continue;
        }
        if(!g_next_alien_collision_side_left) {
            // collide with right side only
            // TODO: Find a better way to collide only with one side
            if(out_of_bounds(alien, -1,
                             g_screen_info.width_in_pixels - 1, -1,
                             g_screen_info.height_in_pixels + 1)) {
                g_next_alien_collision_side_left = true;
                should_update = true;
                break;
            }
        } else {
            // collide with left side only
            // TODO: Find a better way to collide only with one side
            if(out_of_bounds(alien, 1, g_screen_info.width_in_pixels + 1,
                             -1, g_screen_info.height_in_pixels + 1)) {
                g_next_alien_collision_side_left = false;
                should_update = true;
                break;
            }
        }
    }

    if(should_update) {
        for(uint64 i = 0; i < array_get_size(&g_aliens); i++) {
        // TODO: Fix ugly cast with beautiful macro?
        entity_id alien = *((entity_id*)array_get(&g_aliens,i));
            if(!entity_valid(alien)) {
                continue;
            }
            directions[alien.index].x *= -1;
            positions[alien.index].y += MAX(g_screen_info.pixel_size/2.0, 1U);
        }
    }
}

// TODO: Add a "remove_dead_aliens" functions
// which I would call at the beginning of the loop
// and remove all other explicit dead alien checks from code
void game_main(void)
{
    game_init(128, 130, 8);
    write_sin_wave(g_sound_buffer_shoot, 300, 1600);
    write_sin_wave(g_sound_buffer_kill, 500, 1600);
    float64 last_measurement = get_time_ms();
    // TODO: What should be the first value of delta?
    float64 delta = 0;
    g_spaceship_id = create_spaceship();
    create_alien_matrix();
    float64 avg_fps = 0;
    float64 alien_projectile_timer = g_alien_projectile_frequency;
    while(!keys[KEY_ESC].pressed) {

        // reset game state if R is pressed
        if(keys[KEY_R].pressed) {
            reset_game_state(&delta, &last_measurement);
        }

        // check if the game has ended
        if(player_won()) {
            clear_screen(g_backbuffer, COLOR_GREEN);
            poll_platform_messages();
            display_backbuffer(g_backbuffer, g_window);
            continue;
        }

        if(player_lost()) {
            clear_screen(g_backbuffer, COLOR_RED);
            poll_platform_messages();
            display_backbuffer(g_backbuffer, g_window);
            continue;
        }

        clear_screen(g_backbuffer, COLOR_BLACK);
        poll_platform_messages();
        process_input(delta);
        update_alien_positions();
        update_entity_positions(delta);
        generate_alien_projectile(delta, &alien_projectile_timer);
        check_collisions();
        draw_sprites(g_backbuffer);
        display_backbuffer(g_backbuffer, g_window);

        float64 current_measurement = get_time_ms();
        float64 elapsed_time_ms = current_measurement - last_measurement;
        delta = elapsed_time_ms / 1000;
        // convert to ms
        last_measurement = current_measurement;
        // printf("%f ms, %f fps\n", elapsed_time_ms, 1000.0 /
        // (elapsed_time_ms));
        float64 fps = 1000.0 / elapsed_time_ms;
        if(avg_fps == 0) {
            avg_fps = fps;
        } else {
            avg_fps += fps;
            avg_fps /= 2;
        }
    }
    printf("Avg fps: %f\n", avg_fps);
    game_destroy();
}
