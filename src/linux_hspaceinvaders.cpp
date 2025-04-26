#include "include/defines.h"
#include "include/hspaceinvaders.h"
#include "include/input.h"
#include "include/logging.h"
#include "include/platform_layer.h"
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <alsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> //TODO: Delete later
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
    snd_pcm_t* pcm_device_handle;
    snd_pcm_hw_params_t* snd_hw_params;
    uint32 max_hw_frames;
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
                const uint8 bits_per_sample)
{
    LOG_TRACE("Initializing sound subsystem\n");
    int32 ret;

    // Open PCM device for playback
    // TODO: Mode = 0 (default). See if I want to use ASYNC or NONBLOCK
    // Reference: https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html
    // TODO: Find out what name to use. My headphoes change hw:X,Y when plugging
    // them back in.
    // TODO: Opened just for playback now, maybe I want to implement recording
    // at some point as well?
    ret = snd_pcm_open(&g_linux_context.pcm_device_handle, "plughw:3,0",
                       SND_PCM_STREAM_PLAYBACK, 0);
    if(ret < 0) {
        LOG_ERROR("Unable to open pcm device: %s\n", snd_strerror(ret));
    }

    // Allocate a hardware parameters object
    snd_pcm_hw_params_malloc(&g_linux_context.snd_hw_params);
    if(g_linux_context.snd_hw_params == NULL) {
        LOG_ERROR("Alsa: Could not allocate sound params\n");
    }

    // Fill it in with default values
    ret = snd_pcm_hw_params_any(g_linux_context.pcm_device_handle,
                                g_linux_context.snd_hw_params);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error setting default parameters: %s\n",
                  snd_strerror(ret));
    }

    /* Set the desired hardware parameters */

    // Interleaved mode
    ret = snd_pcm_hw_params_set_access(g_linux_context.pcm_device_handle,
                                       g_linux_context.snd_hw_params,
                                       SND_PCM_ACCESS_RW_INTERLEAVED);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error setting access parameters: %s\n",
                  snd_strerror(ret));
    }

    snd_pcm_format_t format;
    if(bits_per_sample == 16) {
        format = SND_PCM_FORMAT_S16_LE;
    } else {
        LOG_ERROR("Sound: Error, unknown PCM format\n");
    }
    ret = snd_pcm_hw_params_set_format(g_linux_context.pcm_device_handle,
                                       g_linux_context.snd_hw_params, format);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error setting audio format: %s\n", snd_strerror(ret));
    }

    // Two channels (stereo)
    snd_pcm_hw_params_set_channels(g_linux_context.pcm_device_handle,
                                   g_linux_context.snd_hw_params, nb_channels);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error configuring channels: %s\n", snd_strerror(ret));
    }

    int32 dir;
    uint32 val = nb_samples_per_sec;
    ret = snd_pcm_hw_params_set_rate_near(g_linux_context.pcm_device_handle,
                                          g_linux_context.snd_hw_params, &val,
                                          &dir);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error setting sampling rate: %s\n", snd_strerror(ret));
    }

    // TODO: Is it OK if I already get max frames before calling
    // snd_pcm_hw_params?
    snd_pcm_uframes_t max_frames;
    snd_pcm_hw_params_get_buffer_size_max(g_linux_context.snd_hw_params,
                                          &max_frames);
    LOG_TRACE("Maximum  number of frames: %ld\n", max_frames);
    // TODO: Figure out what buffer size I want to use
    const uint32 period_size = 1000;
    max_frames = max_frames < period_size ? max_frames : period_size;
    g_linux_context.max_hw_frames = max_frames;
    ret = snd_pcm_hw_params_set_period_size_near(
        g_linux_context.pcm_device_handle, g_linux_context.snd_hw_params,
        &max_frames, &dir);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error setting number of frames: %s\n",
                  snd_strerror(ret));
    }

    // Write the parameters to the driver
    ret = snd_pcm_hw_params(g_linux_context.pcm_device_handle,
                            g_linux_context.snd_hw_params);
    if(ret < 0) {
        LOG_ERROR("Alsa: Unable to set hw parameters: %s\n", snd_strerror(ret));
    }

    uint32 max_period_us;
    ret = snd_pcm_hw_params_get_period_time_max(g_linux_context.snd_hw_params,
                                                &max_period_us, &dir);
    if(ret < 0) {
        LOG_ERROR("Alsa: Unable to get period time max: %s\n",
                  snd_strerror(ret));
    }
    LOG_TRACE("Max period size: %f ms\n", max_period_us / 1000.0f);

    snd_pcm_state_t state = snd_pcm_state(g_linux_context.pcm_device_handle);
    if(state != SND_PCM_STATE_PREPARED) {
        LOG_ERROR(
            "Alsa: PCM device not in prepared state after initalization\n");
    }

    uint32 samples;
    ret = snd_pcm_hw_params_get_rate(g_linux_context.snd_hw_params, &samples,
                                     &dir);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error getting sampling rate: %s\n", snd_strerror(ret));
    }
    LOG_TRACE("Rate: %d\n", samples);

    ret = snd_pcm_hw_params_get_format(g_linux_context.snd_hw_params, &format);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error getting format: %s\n", snd_strerror(ret));
    }

    LOG_TRACE("Format: %d\n", format);

    // NOTE: This is called automatically by snd_pcm_hw_params()
    // snd_pcm_prepare(g_linux_context.pcm_device_handle);
    LOG_TRACE("Sound subsystem successfully initialized\n");
}

platform_sound_buffer* create_sound_buffer(uint32 size_frames)
{
    LOG_TRACE("Creating sound buffer.\n");
    int32 dir;
    int32 ret;
    platform_sound_buffer* sound_buffer = new platform_sound_buffer;
    sound_buffer->context = new platform_sound_buffer_context;

    uint32 val;
    ret = snd_pcm_hw_params_get_period_time(g_linux_context.snd_hw_params, &val,
                                            &dir);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error getting period time: %s\n", snd_strerror(ret));
    }

    ret = snd_pcm_hw_params_get_rate(g_linux_context.snd_hw_params,
                                     &sound_buffer->nb_samples_per_sec, &dir);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error getting sampling rate: %s\n", snd_strerror(ret));
    }

    snd_pcm_format_t format;
    ret = snd_pcm_hw_params_get_format(g_linux_context.snd_hw_params, &format);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error getting format: %s\n", snd_strerror(ret));
    }

    // TODO: See if I want to support other formats
    if(format == SND_PCM_FORMAT_U16_LE || format == SND_PCM_FORMAT_S16_LE) {
        sound_buffer->bits_per_sample = 16;
    } else {
        LOG_ERROR("Sound subsystem: Unknown format\n");
    }

    uint32 channels;
    ret = snd_pcm_hw_params_get_channels(g_linux_context.snd_hw_params,
                                         &channels);
    sound_buffer->nb_channels = (uint8)channels;

    // FIXME: Replace 4 with actual computation using knwon values
    uint32 size = size_frames * 4; /* 2 bytes/sample, 2 channels */
    sound_buffer->buffer = (void*)malloc(size * sizeof(uint8));
    sound_buffer->size_bytes = size;
    sound_buffer->size_frames = size_frames;

    LOG_TRACE("Sound subsystem: Period of buffer: %f ms\n", val / 1000.0f);
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
    uint32 played_frames = 0;
    void* buffer = sound_buffer->buffer;

    while(played_frames < sound_buffer->size_frames) {
        uint32 frames_to_play = (sound_buffer->size_frames - played_frames)
                                        < g_linux_context.max_hw_frames
                                    ? (sound_buffer->size_frames
                                       - played_frames)
                                    : g_linux_context.max_hw_frames;
        int32 ret;
        frames_to_play = frames_to_play < 400 ? frames_to_play : 400;
        ret = snd_pcm_writei(g_linux_context.pcm_device_handle, buffer,
                             frames_to_play);
        // Move forward in the buffer by the number of frames
        // 1 frame = 2 samples, 1 sample = 2 bytes
        buffer = (void*)((char*)buffer + frames_to_play * 4);
        if(ret == -EPIPE) {
            // EPIPE means underrun
            // TODO: Log and see what I want to do here
            LOG_WARNING("Alsa: Underrun occurred\n");
            // FIXME: Do a prepare after an underrun?
        } else if(ret < 0) {
            LOG_ERROR("Alsa: Error from writei: %s\n", snd_strerror(ret));
        } else if(ret != (int32)frames_to_play) {
            LOG_WARNING("Alsa: Short write, wrote %d frames\n", ret);
        }

        // FIXME: Probably want to exit on underrun or make sure that I don't
        // add it here
        played_frames += ret;
    }
}

void teardown_sound()
{
    LOG_TRACE("Tearing down sound subsystem\n");
    // TODO: Maybe I don't need drain
    snd_pcm_drain(g_linux_context.pcm_device_handle);
    snd_pcm_close(g_linux_context.pcm_device_handle);
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

static void test_sound()
{
    long loops;
    int rc;
    int size;
    snd_pcm_t* handle;
    snd_pcm_hw_params_t* params;
    unsigned int val;
    int dir;
    snd_pcm_uframes_t frames;
    char* buffer;
    int ret;

    /* Open PCM device for playback. */
    rc = snd_pcm_open(&handle, "plughw:3,0", SND_PCM_STREAM_PLAYBACK, 0);
    if(rc < 0) {
        fprintf(stderr, "unable to open pcm device: %s\n", snd_strerror(rc));
        exit(1);
    }

    /* Allocate a hardware parameters object. */
    snd_pcm_hw_params_alloca(&params);

    /* Fill it in with default values. */
    snd_pcm_hw_params_any(handle, params);

    /* Set the desired hardware parameters. */

    /* Interleaved mode */
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);

    /* Signed 16-bit little-endian format */
    snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);

    /* Two channels (stereo) */
    snd_pcm_hw_params_set_channels(handle, params, 2);

    /* 44100 bits/second sampling rate (CD quality) */
    val = 44100;
    snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);

    /* Set period size to 32 frames. */
    frames = 32;
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

    /* Write the parameters to the driver */
    rc = snd_pcm_hw_params(handle, params);
    if(rc < 0) {
        fprintf(stderr, "unable to set hw parameters: %s\n", snd_strerror(rc));
        exit(1);
    }

    /* Use a buffer large enough to hold one period */
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    size = frames * 4; /* 2 bytes/sample, 2 channels */
    buffer = (char*)malloc(size);

    /* We want to loop for 5 seconds */
    snd_pcm_hw_params_get_period_time(params, &val, &dir);
    /* 5 seconds in microseconds divided by
     * period time */
    loops = 5000000 / val;
    ret = snd_pcm_hw_params_get_rate(params, &val, &dir);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error getting sampling rate: %s\n", snd_strerror(ret));
    }

    LOG_TRACE("Rate: %d\n", val);
    snd_pcm_format_t format;
    ret = snd_pcm_hw_params_get_format(params, &format);
    if(ret < 0) {
        LOG_ERROR("Alsa: Error getting format: %s\n", snd_strerror(ret));
    }
    LOG_TRACE("Format: %d\n", format);

    while(loops > 0) {
        loops--;
        rc = read(0, buffer, size);
        if(rc == 0) {
            fprintf(stderr, "end of file on input\n");
            break;
        } else if(rc != size) {
            fprintf(stderr, "short read: read %d bytes\n", rc);
        }
        rc = snd_pcm_writei(handle, buffer, frames);
        if(rc == -EPIPE) {
            /* EPIPE means underrun */
            fprintf(stderr, "underrun occurred\n");
            snd_pcm_prepare(handle);
        } else if(rc < 0) {
            fprintf(stderr, "error from writei: %s\n", snd_strerror(rc));
        } else if(rc != (int)frames) {
            fprintf(stderr, "short write, write %d frames\n", rc);
        }
    }

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    free(buffer);

    exit(0);
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
    // int n = 3;
    // int p = 2;
    // ASSERT(3 == 1, "N is %d and P is %d\n", n, p);
    //    test_sound();
    // TODO: Maybe use this to identify the sound device to use?

    // int card = -1;
    // const char* iface = "ctl";
    // void** hints;
    // ret = snd_device_name_hint(card, iface, &hints);
    // printf("Ret: %d\n", ret);

    // int i = 0;
    // if(hints == NULL) {
    //  return -1;
    // }
    // while(hints[i] != NULL) {
    //  char* res = snd_device_name_get_hint(hints[i], "NAME");
    //  char* res2 = snd_device_name_get_hint(hints[i], "DESC");
    //  int idx = snd_card_get_index(res);
    //  snd_ctl_t* ctlp;
    //  // TODO: Use ASYNC instead of nonblock?
    //  ret = snd_ctl_open(&ctlp, res, SND_CTL_ASYNC);
    //  printf("Ret open card: %d\n", ret);
    //  if(res != NULL) {
    //      printf("Name: %s\nDesc: %s\nIndex: %d\n", res, res2, idx);
    //  }
    //  free(res);
    //  free(res2);
    //  i++;
    // }
    //

    game_main();
    return 0;
}
