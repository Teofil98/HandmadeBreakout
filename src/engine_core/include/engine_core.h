// clang-format Language: C
#ifndef ENGINE_CORE_H
#define ENGINE_CORE_H

#include "../../my_lib/defines.h"
#include "platform_layer.h"

typedef struct screen_information {
    uint32 width_in_pixels;
    uint32 height_in_pixels;
    uint32 pixel_size;
    uint32 screen_width;
    uint32 screen_height;
} screen_information;

void init_screen_info(screen_information* info, const uint32 width_in_px,
                      const uint32 height_in_px, const uint32 px_size);
void draw_pixel(platform_backbuffer* backbuffer, const uint32 row,
                const uint32 col, const uint32 color,
                const screen_information* info);

void write_square_wave(platform_sound_buffer* buffer, uint32 frequency,
                       int32 tone_volume);
void write_sin_wave(platform_sound_buffer* buffer, const uint32 frequency,
                    const int32 tone_volume);

static inline uint32 get_frames_from_time_sec(const float32 time,
                                              const uint32 samples_per_second)
{
    return (uint32)(time * samples_per_second);
}

#endif // ENGINE_CORE_H
