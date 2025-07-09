#pragma once

#include "../engine_core/include/platform_layer.h"

void game_main(void);
void game_init(const uint32 width_in_pixels, const uint32 height_in_pixels, const uint32 pixel_size);
void game_destroy(void);

void process_input(float64 delta);
