// clang-format Language: C
#ifndef ECS_ENGINE_H
#define ECS_ENGINE_H

#include "../../my_lib/defines.h"
#include "entities.h"

// User defined functions
void on_init();
void on_update(const float64 delta);
void on_cleanup();

void engine_start();
void engine_init(const uint32 width_in_pixels, const uint32 height_in_pixels,
                 const uint32 pixel_size, screen_information* screen_info);
void engine_quit();

void clear_screen(uint32 color);
void draw_sprite(const entity_id id, screen_information* screen_info);
void draw_sprites(screen_information* screen_info);

#endif // ECS_ENGINE_H
