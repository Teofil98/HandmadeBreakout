#include "include/hspaceinvaders.h"
#include "include/defines.h"
#include "include/logging.h"
#include "include/platform_layer.h"
#include "include/input.h"
#include <math.h> // TODO: replace functions here with own implementation

// GENERAL TODO: Check all return values of all functions and log errors where
// needed. I Probably missed a few places so far in ALL SOURCE FILES SO FAR :)

struct screen_information {

    screen_information(uint32 wip, uint32 hip, uint32 ps)
        : width_in_pixels { wip }, height_in_pixels { hip }, pixel_size { ps }
    {
        screen_width = wip * ps;
        screen_height = hip * ps;
    };

    uint32 width_in_pixels;
    uint32 height_in_pixels;
    uint32 pixel_size;
    uint32 screen_width;
    uint32 screen_height;
};

struct object {

    object(const uint8 w, const uint8 h, const uint8* sprite, uint32 color)
        : width { w }, height { h }, sprite { sprite }, color { color },
          row { 0 }, col { 0 } {};

    const uint32 width;
    const uint32 height;
    const uint8* sprite;
    uint32 color;
    // Current position of sprite
    float32 row;
    float32 col;
};

static screen_information* g_screen_info;

static const uint8_t spaceship_bytes[] = {
    "-----*-----"
    "----***----"
    "--*-*-*-*--"
    "-**-*-*-**-"
    "***********"
    "***********"
    "-**-***-**-"
    "--*--*--*--"
};
static object spaceship(11, 8, spaceship_bytes, RGBA(255, 255, 255, 0));

static const uint8_t alien_bytes[] = {
    "--*----*--"
    "---*--*---"
    "--******--"
    "-**-**-**-"
    "**********"
    "-********-"
    "--**--**--"
    "--*----*--"
};
static object alien(10, 8, alien_bytes, RGBA(255, 255, 255, 0));
static float32 g_spaceship_speed = 30;

static void draw_pixel(platform_backbuffer* backbuffer, const uint32 row,
                       const uint32 col, const uint32 color)
{
    ASSERT(row < g_screen_info->height_in_pixels,
           "Attempting to draw pixel on row %d with a screen height of %d\n",
           row, g_screen_info->height_in_pixels);
    ASSERT(col < g_screen_info->width_in_pixels,
           "Attempting to draw pixel on col %d with a screen width of %d\n",
           col, g_screen_info->width_in_pixels);

    uint32_t* pixels = (uint32_t*)backbuffer->bitmap;
    const uint32 nb_cols = backbuffer->width;

    const uint32_t screen_row = row * g_screen_info->pixel_size;
    const uint32_t screen_col = col * g_screen_info->pixel_size;

    for(uint32 i = screen_row; i < screen_row + g_screen_info->pixel_size;
        i++) {
        for(uint32 j = screen_col; j < screen_col + g_screen_info->pixel_size;
            j++) {
            pixels[i * nb_cols + j] = color;
        }
    }
}

static void draw_sprite(const object* obj, platform_backbuffer* backbuffer)
{
    for(uint32 i = 0; i < obj->height; i++) {
        for(uint32 j = 0; j < obj->width; j++) {
            if(obj->sprite[obj->width * i + j] == '*') {
                draw_pixel(backbuffer, (int32)obj->row + i, (int32)obj->col + j,
                           obj->color);
            }
        }
    }
}

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

static uint32 get_frames_from_time_sec(float32 time, uint32 samples_per_second)
{
    return (uint32)(time * samples_per_second);
}

void game_init(const uint32 width_in_pixels, const uint32 height_in_pixels,
               const uint32 pixel_size)
{
    LOG_TRACE("Game init\n");

    uint32_t bytes_per_pixel = 4;

    g_screen_info = new screen_information(width_in_pixels, height_in_pixels,
                                           pixel_size);
    g_window = open_window(WINDOW_TITLE, g_screen_info->screen_width,
                           g_screen_info->screen_height);
    g_backbuffer = create_backbuffer(g_screen_info->screen_width,
                                     g_screen_info->screen_height,
                                     bytes_per_pixel);
    const uint8 channels = 2;
    const uint32 nb_samples_per_sec = 44100;
    const uint8 bits_per_sample = 16;
    init_sound(channels, nb_samples_per_sec, bits_per_sample);
    // FIXME: No output for 1 second
    g_sound_buffer = create_sound_buffer(
        get_frames_from_time_sec(1.0f, nb_samples_per_sec));
    init_input();
    LOG_TRACE("Game init done\n");
}

void game_destroy(void)
{
    LOG_TRACE("Destroying game\n");
    destroy_window(g_window);
    destroy_backbuffer(g_backbuffer);
    destroy_sound_buffer(g_sound_buffer);
    delete g_screen_info;
    teardown_sound();
    teardown_input();
    LOG_TRACE("Destroyed game\n");
}

void process_input(float64 delta)
{
    if(keys[KEY_A].held) {
        if(spaceship.col - delta * g_spaceship_speed > 0) {
            spaceship.col -= delta * g_spaceship_speed;
        }
    } else if(keys[KEY_D].held) {
        if(spaceship.col + delta * g_spaceship_speed
           < g_screen_info->width_in_pixels - spaceship.width + 1) {
            spaceship.col += delta * g_spaceship_speed;
        }
    }
}

void clear_screen(platform_backbuffer* backbuffer)
{
    uint32* pixels = (uint32_t*)backbuffer->bitmap;
    uint32 num_cols = backbuffer->width;
    for(uint32 i = 0; i < backbuffer->height; i++) {
        for(uint32 j = 0; j < backbuffer->width; j++) {
            pixels[i * num_cols + j] = RGBA(0, 0, 0, 0);
        }
    }
}

void game_main(void)
{
    game_init(128, 128, 8);
    // Used for gradient animation
    int32 xoffset = 0;
    int32 yoffset = 0;
    write_sin_wave(g_sound_buffer, 300, 1600);
    play_sound_buffer(g_sound_buffer);
    float64 last_measurement = get_time_ms();
    float64 delta = 1;
    spaceship.row = 100;
    spaceship.col = 64;
    alien.row = 32;
    alien.col = 64;
    while(!keys[KEY_ESC].pressed) {
        clear_screen(g_backbuffer);
        poll_platform_messages();
        process_input(delta);
        draw_sprite(&spaceship, g_backbuffer);
        draw_sprite(&alien, g_backbuffer);
        // draw_gradient(g_backbuffer, xoffset, yoffset++);
        display_backbuffer(g_backbuffer, g_window);

        float64 current_measurement = get_time_ms();
        float64 elapsed_time_ms = current_measurement - last_measurement;
        delta = elapsed_time_ms / 1000;
        // convert to ms
        last_measurement = current_measurement;
        // printf("%f ms, %f fps\n", elapsed_time_ms, 1000.0 / (elapsed_time_ms));
    }
    game_destroy();
}
