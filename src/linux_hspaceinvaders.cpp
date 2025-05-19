#include "include/defines.h"
#include "include/hspaceinvaders.h"
#include "include/input.h"
#include "include/logging.h"
#include "include/platform_layer.h"
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static Display* g_display_server;
static uint64 g_screen;
static Window g_root_window;

// initial window position
// TODO: See if I want to add something like this in windows
// Also see if I actually need them here or in defines.h
#define POSX 500
#define POSY 500
#define BORDER_W 15
#define PIXEL_SIZE 4

struct platform_window_context {
    GC gc;
    Window window;
};

struct platform_backbuffer_context {
    XImage* image;
};

struct linux_context {
    pa_simple* sound_stream;
    uint32 max_hw_frames;
    uint16 audio_nb_channels;
    uint32 audio_nb_samples_per_sec;
    uint8 audio_bits_per_sample;
};

struct platform_sound_buffer_context {
};

static linux_context g_linux_context;

// define extern variable from input.h
key_state keys[NUM_KEYS];

platform_window* open_window(const char* title, const uint32 width,
                             const uint32 height)
{
    LOG_TRACE("Opening window\n");
    g_display_server = XOpenDisplay(NULL);
    if(g_display_server == NULL) {
        LOG_ERROR("Can't open display");
    }

    g_screen = DefaultScreen(g_display_server);
    g_root_window = RootWindow(g_display_server, g_screen);

    XSetWindowAttributes attributes;
    attributes.background_pixel = WhitePixel(g_display_server, g_screen);
    attributes.border_pixel = BlackPixel(g_display_server, g_screen);
    attributes.event_mask = KeyReleaseMask | KeyPressMask;

    // TODO: Change from DefaultDepth to something which includes the alpha
    // channel as well
    Window window = XCreateWindow(
        g_display_server, g_root_window, POSX, POSY, width, height, BORDER_W,
        DefaultDepth(g_display_server, g_screen), InputOutput,
        DefaultVisual(g_display_server, g_screen),
        CWBackPixel | CWBorderPixel | CWEventMask, &attributes);

    GC gc = XCreateGC(g_display_server, window, 0, NULL);
    XAutoRepeatOff(g_display_server);

    XTextProperty window_title_propery;
    uint32 title_size = strlen(title);
    char* title_cpy = (char*)malloc(sizeof(int8) * (title_size + 1));
    strncpy(title_cpy, title, title_size);
    title_cpy[title_size] = '\0';
    if(XStringListToTextProperty(&title_cpy, 1, &window_title_propery) == 0) {
        LOG_ERROR("Error creating window title property\n");
    }
    XSetWMName(g_display_server, window, &window_title_propery);
    // TODO: See if I want a different name for icon title
    XSetWMIconName(g_display_server, window, &window_title_propery);
    // Make window visible*/
    XFree(window_title_propery.value);
    XMapWindow(g_display_server, window);

    platform_window* plat_window = new platform_window;
    plat_window->context = new platform_window_context;
    plat_window->title = title_cpy;
    plat_window->context->gc = gc;
    plat_window->context->window = window;

    return plat_window;
}

void destroy_window(platform_window* window)
{
    free(window->title);

    XUnmapWindow(g_display_server, window->context->window);
    XDestroyWindow(g_display_server, window->context->window);
    XCloseDisplay(g_display_server);
}

platform_backbuffer* create_backbuffer(const uint32 width, const uint32 height,
                                       const uint32 bytes_per_pixel)
{
    // create image buffer which will be used to blit to screen
    // TODO: Change from DefaultDepth to something which includes the alpha
    // channel as well
    // FIXME: Why is this working with 4 if by default I only have 24 bits????
    // (Could be the bitmap_pad)
    // FIXME: Figure out how to make a window with depth of 32

    // NOTE: bits_per_pixel = 32 already, I guess the 8 bits get ignored by
    // default for the 24 depth window??
    char* backbuffer = (char*)malloc(bytes_per_pixel * height * width);
    XImage* image = XCreateImage(
        g_display_server, DefaultVisual(g_display_server, g_screen),
        DefaultDepth(g_display_server, g_screen), ZPixmap, 0, backbuffer, width,
        height,
        32, // TODO: probably want to change this bitmap_pad to 32 once I get
            // the depth sorted out
        0);

    platform_backbuffer* plat_backbuffer = new platform_backbuffer;
    plat_backbuffer->context = new platform_backbuffer_context;
    plat_backbuffer->height = height;
    plat_backbuffer->width = width;
    plat_backbuffer->bytes_per_pixel = bytes_per_pixel;
    plat_backbuffer->bitmap = (void*)backbuffer;
    plat_backbuffer->context->image = image;

    return plat_backbuffer;
}

void destroy_backbuffer(platform_backbuffer* backbuffer)
{
    XDestroyImage(backbuffer->context->image);
    delete backbuffer->context;
    delete backbuffer;
}

void display_backbuffer(const platform_backbuffer* backbuffer,
                        const platform_window* window)
{
    // TODO: Check if XCreateImage and XPutImage let me efficiently blit to
    // screen
    XPutImage(g_display_server, window->context->window, window->context->gc,
              backbuffer->context->image, 0, 0, 0, 0, backbuffer->width,
              backbuffer->height);
}

static status convert_x11_key(KeySym key, key_id *k_id)
{
    switch(key) {
        case XK_Escape: {
            *k_id = KEY_ESC;
        } break;
        case XK_space: {
            *k_id = KEY_SPACE;
        } break;
        case XK_w: {
            *k_id = KEY_W;
        } break;
        case XK_a: {
            *k_id = KEY_A;
        } break;
        case XK_s: {
            *k_id = KEY_S;
        } break;
        case XK_d: {
            *k_id = KEY_D;
        } break;
        case XK_r: {
            *k_id = KEY_R;
        } break;
        default:
            LOG_WARNING("Unknown X11 key: %lx\n", key);
            return STATUS_FAILURE;
    }
    return STATUS_SUCCESS;
}

static void press_key(KeySym key)
{
    key_id k_id;
    if(convert_x11_key(key, &k_id) == STATUS_FAILURE) {
        return;
    }

    if(!keys[k_id].held) {
        keys[k_id].pressed = true;
    }
    keys[k_id].held = true;
}

static void release_key(KeySym key)
{
    key_id k_id;
    if(convert_x11_key(key, &k_id) == STATUS_FAILURE) {
        return;
    }

    keys[k_id].held = false;
    keys[k_id].released = true;
}

void poll_platform_messages(void)
{
    // reset all keys that were pressed or released the previous frames
    for(int i = 0; i < NUM_KEYS; i++) {
        keys[i].released = false;
        keys[i].pressed = false;
    }

    XEvent event;
    // TODO: Figure out what KeymapStateMask does and if I need it in addition
    // to KeyReleaseMask
    long event_mask = KeyPressMask | KeyReleaseMask;
    if(XCheckMaskEvent(g_display_server, event_mask, &event)) {
        // TODO: Figure out what last 2 parameters do (group and level)
        KeySym key = XkbKeycodeToKeysym(g_display_server, event.xkey.keycode, 0,
                                        0);

        switch(event.type) {
            case KeyPress: {
                press_key(key);
            } break;
            case KeyRelease: {
                release_key(key);
            } break;
            default:
                LOG_WARNING("Unknown event type: %d\n", event.xkey.type);
        }
    }
}

void init_sound(const uint16 nb_channels, const uint32 nb_samples_per_sec,
                const uint8 bits_per_sample, const char* name)
{
    LOG_TRACE("Initializing sound subsystem\n");

    pa_sample_format format;
    if(bits_per_sample == 16) {
        // TODO: Do I want to also support the big endian case?
        format = PA_SAMPLE_S16LE;
    } else {
        LOG_ERROR("Sound: Error, unknown PCM format (bits per sample)\n");
    }
    // Configure sample format
    const pa_sample_spec ss = { .format = format,
                                .rate = nb_samples_per_sec,
                                .channels = (uint8)nb_channels };

    int32 error;
    /* Create a new playback stream */
    g_linux_context.sound_stream = pa_simple_new(NULL, name, PA_STREAM_PLAYBACK,
                                                 NULL, "playback", &ss, NULL,
                                                 NULL, &error);
    if(g_linux_context.sound_stream == NULL) {
        LOG_ERROR("Sound: pa_simple_new() failed: %s\n", pa_strerror(error));
    }

    // save config information
    g_linux_context.audio_bits_per_sample = bits_per_sample;
    g_linux_context.audio_nb_samples_per_sec = nb_samples_per_sec;
    g_linux_context.audio_nb_channels = nb_channels;

    LOG_TRACE("Sound subsystem successfully initialized\n");
}

platform_sound_buffer* create_sound_buffer(uint32 size_frames)
{
    LOG_TRACE("Creating sound buffer.\n");
    platform_sound_buffer* sound_buffer = new platform_sound_buffer;
    sound_buffer->context = new platform_sound_buffer_context;


    sound_buffer->nb_samples_per_sec = g_linux_context.audio_nb_samples_per_sec;
    sound_buffer->bits_per_sample = g_linux_context.audio_bits_per_sample;
    sound_buffer->nb_channels = g_linux_context.audio_nb_channels;

    // FIXME: Replace 4 with actual computation using knwon values
    uint32 size = size_frames * 4; /* 2 bytes/sample, 2 channels */
    sound_buffer->buffer = (void*)malloc(size * sizeof(uint8));
    sound_buffer->size_bytes = size;
    sound_buffer->size_frames = size_frames;

    LOG_TRACE("Created sound buffer\n");
    return sound_buffer;
}

void destroy_sound_buffer(platform_sound_buffer* sound_buffer)
{
    LOG_TRACE("Destroying sound buffer\n");
    free(sound_buffer->buffer);
    delete sound_buffer->context;
    delete sound_buffer;
    LOG_TRACE("Destroyed sound buffer\n");
}

// FIXME: The while loop causes delay for displaying graphics
// Maybe want to have this as a separate thread
void play_sound_buffer(platform_sound_buffer* sound_buffer)
{
    int error = 0;
    int ret;
    ret = pa_simple_write(g_linux_context.sound_stream,  sound_buffer->buffer, sound_buffer->size_bytes, &error);

    if(ret < 0) {
        LOG_ERROR("Error on pa_simple_write(): %s\n", pa_strerror(error));
    }
}

void teardown_sound()
{
    LOG_TRACE("Tearing down sound subsystem\n");
    pa_simple_drain(g_linux_context.sound_stream, NULL);
    pa_simple_free(g_linux_context.sound_stream);
    LOG_TRACE("Sound subsystem closed\n");
}

void init_input(void)
{
    LOG_TRACE("Initializing input subsystem\n");
    for(int i = 0; i < NUM_KEYS; i++) {
        keys[i].pressed = false;
        keys[i].held = false;
        keys[i].released = false;
    }
    LOG_TRACE("Input subsystem successfully initialized\n");
}

void teardown_input(void)
{
    LOG_TRACE("Tearing down input subsystem\n");
    LOG_TRACE("Input subsystem closed\n");
}

float64 get_time_ms(void)
{
    struct timeval te;
    // get current time
    gettimeofday(&te, NULL);
    // convert time to milliseconds
    float64 milliseconds = te.tv_sec * 1000 + te.tv_usec / 1000.0;
    return milliseconds;
}

int main()
{
    game_main();
    return 0;
}
