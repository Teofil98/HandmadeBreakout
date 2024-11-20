#pragma once

#include "defines.h"

struct platform_window_context;
struct platform_window {
    char* title;
    // TODO: See what I want to do with width and height when window is resized
    //uint32 width;
    //uint32 height;
    platform_window_context* context;
};

struct platform_backbuffer_context;
struct platform_backbuffer {
    uint32 bytes_per_pixel;
    void* bitmap;
    uint32 width;
    uint32 height;
    platform_backbuffer_context* context;
};

struct platform_sound_buffer_context;
struct platform_sound_buffer {
    uint32 nb_samples_per_sec;
    uint8 bits_per_sample;
    void* buffer;
    uint32 size_bytes;
    uint8 nb_channels;
	uint32 size_frames;
    platform_sound_buffer_context* context;
};

platform_window* open_window(const char* title, const uint32 width, const uint32 height);
void destroy_window(platform_window* window);

platform_backbuffer* create_backbuffer(const uint32 width, const uint32 height, const uint32 bytes_per_pixel);
void destroy_backbuffer(platform_backbuffer* backbuffer);
void display_backbuffer(const platform_backbuffer* backbuffer, const platform_window* window);

// TODO: Do I want separate init and create buffer functions?
// TODO: Init should probably return an error if it fails
void init_sound(const uint16 nb_channels, const uint32 nb_samples_per_sec, const uint8 bits_per_sample);
platform_sound_buffer* create_sound_buffer(const uint32 size_frames);
void destroy_sound_buffer(platform_sound_buffer* sound_buffer);
void play_sound_buffer(platform_sound_buffer* sound_buffer);
void teardown_sound(void);

void init_input(void);
void teardown_input(void);

float64 get_time_ms(void);

void poll_platform_messages(void);
