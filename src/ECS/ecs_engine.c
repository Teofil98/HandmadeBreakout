#include <stdbool.h>
#include <stdio.h>
#include "include/ecs_engine.h"
#include "../engine_core/include/platform_layer.h"
#include "../my_lib/defines.h"
#include "../my_lib/logging.h"
#include "../my_lib/rand.h"


static bool g_quit = false;
static platform_backbuffer* g_backbuffer;
static platform_window* g_window;


// OPTIMIZE: Memcpy
static void _clear_screen(platform_backbuffer* backbuffer, uint32 color)
{
    uint32* pixels = (uint32_t*)backbuffer->bitmap;
    uint32 num_cols = backbuffer->width;
    for(uint32 i = 0; i < backbuffer->height; i++) {
        for(uint32 j = 0; j < backbuffer->width; j++) {
            pixels[i * num_cols + j] = color;
        }
    }
}

void clear_screen(uint32 color)
{
    _clear_screen(g_backbuffer, color);
}


void _draw_sprite(const entity_id id, platform_backbuffer* backbuffer,
                 screen_information* screen_info)
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
                draw_pixel(backbuffer, entity_row + i, entity_col + j, color,
                           screen_info);
            }
        }
    }
}

void draw_sprite(const entity_id id, screen_information* screen_info)
{
    _draw_sprite(id, g_backbuffer, screen_info);
}

void _draw_sprites(platform_backbuffer* backbuffer,
                  screen_information* screen_info)
{
    for(uint64 i = 0; i < MAX_ENTITIES; i++) {
        entity_id id = entity_id_array[i];
        if(entity_in_use[id.index]) {
            _draw_sprite(id, backbuffer, screen_info);
        }
    }
}


void draw_sprites(screen_information* screen_info)
{
    _draw_sprites(g_backbuffer, screen_info);
}

void engine_init(const uint32 width_in_pixels, const uint32 height_in_pixels,
               const uint32 pixel_size, screen_information* screen_info)
{
    LOG_TRACE("Engine init\n");

    const uint32_t bytes_per_pixel = 4;


    init_screen_info(screen_info, width_in_pixels, height_in_pixels, pixel_size);
    g_window = open_window(WINDOW_TITLE, screen_info->screen_width,
                           screen_info->screen_height);
    g_backbuffer = create_backbuffer(screen_info->screen_width,
                                     screen_info->screen_height,
                                     bytes_per_pixel);
    const uint8 channels = 2;
    const uint32 nb_samples_per_sec = 44100;
    const uint8 bits_per_sample = 16;
    init_sound(channels, nb_samples_per_sec, bits_per_sample, WINDOW_TITLE);
    init_input();
    init_entity_system();

    LOG_TRACE("Engine init done\n");
}

void engine_start()
{
    // TODO: What should be the first value of delta?
    float64 delta = 0;
    float64 avg_fps = 0;
    g_quit = false;

    on_init();
    float64 last_measurement = get_time_ms();
    while(!g_quit) {
        _clear_screen(g_backbuffer, COLOR_BLACK);
        poll_platform_messages();

        on_update(delta);
        display_backbuffer(g_backbuffer, g_window);

        // Measurements
        float64 current_measurement = get_time_ms();
        float64 elapsed_time_ms = current_measurement - last_measurement;
        delta = elapsed_time_ms / 1000;
        last_measurement = current_measurement;
        float64 fps = 1000.0 / elapsed_time_ms;
        if(avg_fps == 0) {
            avg_fps = fps;
        } else {
            avg_fps += fps;
            avg_fps /= 2;
        }

    }
    printf("Avg fps: %f\n", avg_fps);
    on_cleanup();

    // Engine cleanup
	teardown_entity_system();
    destroy_window(g_window);
    destroy_backbuffer(g_backbuffer);
    teardown_sound();
    teardown_input();
}

void engine_quit()
{
    g_quit = true;
}

