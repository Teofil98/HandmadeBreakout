#include "include/engine_core.h"
#include "../my_lib/logging.h"
#include <math.h>

void init_screen_info(screen_information* info, const uint32 width_in_px, const uint32 height_in_px, const uint32 px_size)
{
    info->width_in_pixels = width_in_px;
    info->height_in_pixels = height_in_px;
    info->pixel_size = px_size;
    info->screen_width = width_in_px * px_size;
    info->screen_height = height_in_px * px_size;
}

void draw_pixel(platform_backbuffer* backbuffer, const uint32 row, const uint32 col, const uint32 color, const screen_information* info)
{
    ASSERT(row < info->height_in_pixels,
           "Attempting to draw pixel on row %d with a screen height of %d\n",
           row, info->height_in_pixels);
    ASSERT(col < info->width_in_pixels,
           "Attempting to draw pixel on col %d with a screen width of %d\n",
           col, info->width_in_pixels);

    uint32_t* pixels = (uint32_t*)backbuffer->bitmap;
    const uint32 nb_cols = backbuffer->width;

    const uint32_t screen_row = row * info->pixel_size;
    const uint32_t screen_col = col * info->pixel_size;

    for(uint32 i = screen_row; i < screen_row + info->pixel_size;
        i++) {
        for(uint32 j = screen_col; j < screen_col + info->pixel_size;
            j++) {
            pixels[i * nb_cols + j] = color;
        }
    }
}

// FIXME: Certain frequencies produce audible skip
void write_square_wave(platform_sound_buffer* buffer, const uint32 frequency,
                       const int tone_volume)
{
    // TODO: Consider if I want to have non 16b/sample  audio
    // TODO: xaudio2_buffer->NbBytes should be multiple of 2, maybe assert
    uint16* audio_buffer = (uint16*)buffer->buffer;
    int32 nb_samples = (int32)buffer->size_bytes / 2;
    // TODO:  For now, I assume that the buffer lasts for 1 second
    // FIXME: Deal with buffers that have length more than 1 sec
    const uint32 square_wave_period = nb_samples / frequency;
    const uint32 half_period = square_wave_period / 2;

    for(int i = 0; i < nb_samples; i += 2) {
        int sign = (i / half_period) % 2 == 0 ? 1 : -1;
        // set left and right samples
        audio_buffer[i] = sign * tone_volume;
        audio_buffer[i + 1] = sign * tone_volume;
    }
}

// FIXME: Certain frequencies produce audible skip
void write_sin_wave(platform_sound_buffer* buffer, const uint32 frequency,
                    const int tone_volume)
{
    // TODO: Consider if I want to have non 16b/sample  audio
    // TODO: xaudio2_buffer->NbBytes should be multiple of 2, maybe assert
    uint16* audio_buffer = (uint16*)buffer->buffer;
    int32 nb_samples = (int32)buffer->size_bytes / 2;
    const uint32 wave_period = buffer->nb_samples_per_sec / frequency;

    for(int i = 0; i < nb_samples; i += 2) {
        // Where in the sin wave we are, in radians
        float32 sin_location = 2 * PI
                               * ((i % wave_period) / (float32)wave_period);
        float32 sin_value = sinf(sin_location);
        // set left and right samples
        audio_buffer[i] = (uint16)(sin_value * tone_volume);
        audio_buffer[i + 1] = (uint16)(sin_value * tone_volume);
    }
}
