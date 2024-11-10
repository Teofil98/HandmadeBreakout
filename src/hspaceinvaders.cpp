#include "include/hspaceinvaders.h"
#include "include/defines.h"
#include "include/logging.h"
#include "include/platform_layer.h"
#include <math.h>  // TODO: replace functions here with own implementation
#include <stdio.h> // TODO: Delete once testing done

// GENERAL TODO: Check all return values of all functions and log errors where
// needed. I Probably missed a few places so far in ALL SOURCE FILES SO FAR :)

void draw_gradient(const platform_backbuffer* backbuffer,
                   const uint32 row_offset, const uint32 col_offset)
{
    uint32_t* pixels = (uint32_t*)backbuffer->bitmap;
    const uint32 nb_rows = backbuffer->height;
    const uint32 nb_cols = backbuffer->width;
    for(uint32 row = 0; row < nb_rows; row++) {
        for(uint32 col = 0; col < nb_cols; col++) {
            pixels[(row * nb_cols) + col] = RGBA(0, (uint8)(row + row_offset),
                                                 (uint8)(col + col_offset), 0);
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
    // TODO:  For now, I assume that the buffer lasts for 1 second
    // FIXME: Deal with buffers that have length more than 1 sec
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

static platform_window* g_window;
static platform_backbuffer* g_backbuffer;
static platform_sound_buffer* g_sound_buffer;

static inline uint32 get_frames_from_time_sec(float32 time,
                                              uint32 samples_per_second)
{
    return (uint32)(time * samples_per_second);
}

void game_init(void)
{
    LOG_TRACE("Game init\n");
    g_window = open_window(WINDOW_TITLE, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H);
    g_backbuffer = create_backbuffer(DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, 4);
    const uint8 channels = 2;
    const uint32 nb_samples_per_sec = 44100;
    const uint8 bits_per_sample = 16;
    init_sound(channels, nb_samples_per_sec, bits_per_sample);
    // FIXME: No output for 1 second
    g_sound_buffer = create_sound_buffer(
        get_frames_from_time_sec(1.0f, nb_samples_per_sec));
    LOG_TRACE("Game init done\n");
}

void game_destroy(void)
{
    LOG_TRACE("Destroying game\n");
    destroy_window(g_window);
    destroy_backbuffer(g_backbuffer);
    destroy_sound_buffer(g_sound_buffer);
    LOG_TRACE("Destroyed game\n");
}

void game_main(void)
{
    game_init();
    // Used for gradient animation
    int32 xoffset = 0;
    int32 yoffset = 0;
    write_sin_wave(g_sound_buffer, 300, 1600);
    play_sound_buffer(g_sound_buffer);
    // uint64 last_measurement = get_timer();
    // uint64 timer_freq = get_timer_frequency();
    while(!should_close()) {
        poll_platform_messages();
        draw_gradient(g_backbuffer, xoffset, yoffset++);
        display_backbuffer(g_backbuffer, g_window);
        // FIXME: Implement get_time_ms instead of the timer shenannigans

        //  uint64 current_measurement = get_timer();
        //  uint64 elapsed_time = current_measurement - last_measurement;
        // convert to ms
        // elapsed_time *= 1000;
        // elapsed_time /= timer_freq;
        // last_measurement = current_measurement;
        // printf("%lld ms, %lld fps\n", elapsed_time, 1000/(elapsed_time));
    }
    game_destroy();
}
