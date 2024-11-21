#pragma once

#include "platform_layer.h"

void game_main(void);
void game_init(const uint32 width_in_pixels, const uint32 height_in_pixels, const uint32 pixel_size);
void game_destroy(void);

void draw_gradient(const platform_backbuffer* backbuffer, const uint32 row_offset, const uint32 col_offset);

void write_square_wave(platform_sound_buffer* buffer, const uint32 frequency, const int32 tone_volume);
void write_sin_wave(platform_sound_buffer* buffer, const uint32 frequency, const int32 tone_volume);
void process_input(float64 delta);
