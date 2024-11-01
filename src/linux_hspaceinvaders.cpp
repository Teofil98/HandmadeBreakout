//#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
//#include <X11/extensions/XKB.h>
//#include <X11/keysym.h>
//#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#include <alsa/asoundlib.h>
#include "include/defines.h"
#include "include/platform_layer.h"
#include "include/hspaceinvaders.h"
#include <string.h> //TODO: Delete later
#include <stdio.h>
#include <stdlib.h>

static Display* g_display_server;
static uint64 g_screen;
static Window g_root_window;
static bool g_exit = false;

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
	snd_pcm_hw_params_t* snd_hw_params; 
	snd_pcm_t* pcm_device_handle;
};

struct platform_sound_buffer_context {
	snd_pcm_uframes_t nb_frames;	
};

static linux_context g_linux_context;

platform_window* open_window(const char* title, const uint32 width, const uint32 height)
{
	g_display_server = XOpenDisplay(NULL);
	if(g_display_server == NULL) {
		// TODO: Log error
		printf("Error, can't open display\n");
	}

	g_screen = DefaultScreen(g_display_server);
	g_root_window = RootWindow(g_display_server, g_screen);

	XSetWindowAttributes attributes;
	attributes.background_pixel = WhitePixel(g_display_server, g_screen);
	attributes.border_pixel = BlackPixel(g_display_server, g_screen);
	attributes.event_mask = KeyReleaseMask;

	// TODO: Change from DefaultDepth to something which includes the alpha channel as well
	Window window = XCreateWindow(g_display_server, g_root_window, 
			POSX, POSY, width, height, BORDER_W, 
			DefaultDepth(g_display_server, g_screen), InputOutput, DefaultVisual(g_display_server, g_screen),
			CWBackPixel | CWBorderPixel | CWEventMask, &attributes);

	GC gc = XCreateGC(g_display_server, window, 0, NULL);

	XTextProperty window_title_propery;
	char* title_cpy = (char*)malloc(strlen(title));
	strcpy(title_cpy, title);
	if(XStringListToTextProperty(&title_cpy, 1, &window_title_propery) == 0) {
		// TODO: handle error
		printf("Error creating window title property\n");
	}
	XSetWMName(g_display_server, window, &window_title_propery);
	// TODO: See if I want a different name for icon title 
	XSetWMIconName(g_display_server, window, &window_title_propery);
	// Make window visible
	XMapWindow(g_display_server, window);

	platform_window* plat_window = new platform_window; 
	plat_window->context = new platform_window_context;
	plat_window->title = (char*)malloc(strlen(title));
	strcpy(plat_window->title, title);
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

bool should_close(void)
{
	return g_exit;
}

platform_backbuffer* create_backbuffer(const uint32 width, const uint32 height, const uint32 bytes_per_pixel)
{
	// create image buffer which will be used to blit to screen
	// TODO: Change from DefaultDepth to something which includes the alpha channel as well
	// FIXME: Why is this working with 4 if by default I only have 24 bits???? (Could be the bitmap_pad)
	// FIXME: Figure out how to make a window with depth of 32

	// NOTE: bits_per_pixel = 32 already, I guess the 8 bits get ignored by default for the 24 depth window??
	char* backbuffer = (char*) malloc(bytes_per_pixel * height * width); 
	XImage* image = XCreateImage(g_display_server, DefaultVisual(g_display_server, g_screen),
			DefaultDepth(g_display_server, g_screen), ZPixmap,
			0, backbuffer, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
			32, // TODO: probably want to change this bitmap_pad to 32 once I get the depth sorted out
			0);


	platform_backbuffer* plat_backbuffer = new platform_backbuffer;
	plat_backbuffer->context = new platform_backbuffer_context;
	plat_backbuffer->height = height;
	plat_backbuffer->width = width;
	plat_backbuffer->bytes_per_pixel = bytes_per_pixel;
	plat_backbuffer->bitmap = (void*)backbuffer;
	plat_backbuffer->context->image = image;
	//	printf("Bitmap unit: %d\nBits per pixel: %d\n\n", image->bitmap_unit, image->bits_per_pixel);
	//	printf("Direct Color: %d\n", DirectColor);

	return plat_backbuffer;
}

void destroy_backbuffer(platform_backbuffer* backbuffer)
{
	XDestroyImage(backbuffer->context->image);
	delete backbuffer->context;
	delete backbuffer;
}

void display_backbuffer(const platform_backbuffer* backbuffer, const platform_window* window)
{
	// TODO: CHeck if XCreateImage and XPutImage let me efficiently blit to screen
	XPutImage(g_display_server, window->context->window, window->context->gc, backbuffer->context->image, 
			0, 0, 0, 0,
			backbuffer->width, backbuffer->height);
}

void poll_platform_messages (void)
{
    XEvent event;
    // TODO: Figure out what KeymapStateMask does and if I need it in addition
    // to KeyReleaseMask
    long event_mask = KeyReleaseMask;
    if (XCheckMaskEvent (g_display_server, event_mask, &event)) {
        // TODO: Figure out what last 2 parameters do (group and level)
        KeySym key = XkbKeycodeToKeysym (g_display_server, event.xkey.keycode,
                                         0, 0);
        switch (event.type) {
            case KeyRelease: {
                switch (key) {
                    case XK_Escape: {
                        g_exit = true;
                    } break;
                    case XK_w: {
                        printf ("W\n");
                    } break;
                    case XK_s: {
                        printf ("S\n");
                    } break;
                    default:
                        printf ("Event key type: %lx\n", key);
                }
            } break;
            default:
                printf ("Event type: %d\n", event.xkey.type);
        }
    }
}

void init_sound(const uint16 nb_channels, const uint32 nb_samples_per_sec, const uint8 bits_per_sample)
{
	int32 ret;

	// Open PCM device for playback
	// TODO: Mode = 0 (default). See if I want to use ASYNC or NONBLOCK
	// Reference: https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html
	// TODO: Find out what name to use. My headphoes change hw:X,Y when plugging them back in.
	// TODO: Opened just for playback now, maybe I want to implement recording at some point as well?
	ret = snd_pcm_open(&g_linux_context.pcm_device_handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (ret < 0) {
		printf("Error: unable to open pcm device: %s\n", snd_strerror(ret));
		// TODO: Log error instead
		exit(1);
	}

	// Allocate a hardware parameters object
	snd_pcm_hw_params_alloca(&g_linux_context.snd_hw_params);
	if(g_linux_context.snd_hw_params == NULL) {
		// TODO: Log error
		printf("Alsa: Could not allocate sound params\n");
		exit(1);
	}
	
	// Fill it in with default values
	ret = snd_pcm_hw_params_any(g_linux_context.pcm_device_handle, g_linux_context.snd_hw_params);
	if(ret < 0) {
		// TODO: Log error
		printf("Alsa: Error setting default parameters: %s\n", snd_strerror(ret));
		exit(1);
	}

	/* Set the desired hardware parameters */

	// Interleaved mode
	ret = snd_pcm_hw_params_set_access(g_linux_context.pcm_device_handle, g_linux_context.snd_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
	if(ret < 0) {
		// TODO: Log error
		printf("Alsa: Error setting access parameters: %s\n", snd_strerror(ret));
		exit(1);
	}

	snd_pcm_format_t format;
	if(bits_per_sample == 16) {
		format = SND_PCM_FORMAT_S16_LE;
	} else {
		// TODO: Log error
		printf("Sound: Error, unknown PCM format\n");
		exit(1);
	}
	ret = snd_pcm_hw_params_set_format(g_linux_context.pcm_device_handle, g_linux_context.snd_hw_params, format);
	if(ret < 0) {
		// TODO: Log error
		printf("Alsa: Error setting audio format: %s\n", snd_strerror(ret));
		exit(1);
	}

	// Two channels (stereo) 
	snd_pcm_hw_params_set_channels(g_linux_context.pcm_device_handle, g_linux_context.snd_hw_params, nb_channels);
	if(ret < 0) {
		// TODO: Log error
		printf("Alsa: Error configuring channels: %s\n", snd_strerror(ret));
		exit(1);
	}

	int32 dir;
	uint32 val = nb_samples_per_sec;
	ret = snd_pcm_hw_params_set_rate_near(g_linux_context.pcm_device_handle, g_linux_context.snd_hw_params, &val, &dir);
	if(ret < 0) {
		// TODO: Log error
		printf("Alsa: Error setting sampling rate: %s\n", snd_strerror(ret));
		exit(1);
	}

	// Set period size to 1 second
	snd_pcm_uframes_t frames = nb_samples_per_sec / nb_channels;
	ret = snd_pcm_hw_params_set_period_size_near(g_linux_context.pcm_device_handle, g_linux_context.snd_hw_params, &frames, &dir);
	if(ret < 0) {
		// TODO: Log error
		printf("Alsa: Error setting number of frames: %s\n", snd_strerror(ret));
		exit(1);
	}

	//TODO: Log trace
	uint32 max_period_us;
	snd_pcm_hw_params_get_period_time_max ( g_linux_context.snd_hw_params, &max_period_us, &dir);
	printf("Max period size: %f ms\n", max_period_us/1000.0f);

	// Write the parameters to the driver
	ret = snd_pcm_hw_params(g_linux_context.pcm_device_handle, g_linux_context.snd_hw_params);
	if (ret < 0) {
		// TODO: Log error
		printf("Alsa: Unable to set hw parameters: %s\n", snd_strerror(ret));
		exit(1);
	}

	snd_pcm_state_t state = snd_pcm_state(g_linux_context.pcm_device_handle);
	if(state != SND_PCM_STATE_PREPARED) {
		//TODO: Log error
		printf("Alsa: PCM device not in prepared state after initalization\n");
	}

	//NOTE: This should be called automatically by snd_pcm_hw_params()
    //snd_pcm_prepare(g_linux_context.pcm_device_handle);

}

//FIXME: Change function signature to get size as an argument?
//For now I assume buffer lasts for 1 second
platform_sound_buffer* create_sound_buffer(void)
{
	//TODO: Log infos here
	int ret;
	platform_sound_buffer* sound_buffer = new platform_sound_buffer;
	sound_buffer->context = new platform_sound_buffer_context;

	int32 dir;
	ret = snd_pcm_hw_params_get_rate (g_linux_context.snd_hw_params, &sound_buffer->nb_samples_per_sec, &dir);
	if(ret < 0) {
		// TODO: Log error
		printf("Alsa: Error getting sampling rate: %s\n", snd_strerror(ret));
		exit(1);
	}

	snd_pcm_format_t format; 
	ret = snd_pcm_hw_params_get_format (g_linux_context.snd_hw_params, &format);
	if(ret < 0) {
		// TODO: Log error
		printf("Alsa: Error getting format: %s\n", snd_strerror(ret));
		exit(1);
	}

	// TODO: See if I want to support other formats
	if(format == SND_PCM_FORMAT_U16_LE || format == SND_PCM_FORMAT_S16_LE ) {
		sound_buffer->bits_per_sample = 16;
	} else {
		// TODO: Log error
		printf("Sound subsystem: Unknown format\n");
		exit(1);
	}


	/* Use a buffer large enough to hold one period */
	snd_pcm_uframes_t frames;
	ret = snd_pcm_hw_params_get_period_size(g_linux_context.snd_hw_params, &frames, &dir);
	if (ret < 0) {
		// TODO: Log error
		printf("Alsa: Error getting period size: %s\n", snd_strerror(ret));
		exit(1);
	}

	uint32 size = frames * 4; /* 2 bytes/sample, 2 channels */
	sound_buffer->buffer = (void*) malloc(size * sizeof(uint8));
	sound_buffer->size_bytes = size;
	sound_buffer->context->nb_frames = frames;

	
	uint32 val;
	ret = snd_pcm_hw_params_get_period_time(g_linux_context.snd_hw_params, &val, &dir);
	if (ret < 0) {
		// TODO: Log error
		printf("Alsa: Error getting period time: %s\n", snd_strerror(ret));
		exit(1);
	}
	//TODO: Log trace
	printf("Sound subsystem: Period of buffer: %f ms\n", val/1000.0f);

	uint32 channels; 
	ret = snd_pcm_hw_params_get_channels(g_linux_context.snd_hw_params, &channels);
	sound_buffer->nb_channels = (uint8) channels;  

	return sound_buffer;
}

void destroy_sound_buffer(platform_sound_buffer* sound_buffer)
{
	free(sound_buffer->buffer);
	delete sound_buffer->context;
	delete sound_buffer;
}

void play_sound_buffer(platform_sound_buffer *sound_buffer)
{
	int32 ret;
    ret = snd_pcm_writei(g_linux_context.pcm_device_handle, sound_buffer->buffer, sound_buffer->context->nb_frames);
    if(ret == -EPIPE) {
        // EPIPE means underrun
		// TODO: Log and see what I want to do here
        printf("Alsa: Underrun occurred\n");
    } else if(ret < 0) {
		// TODO: Log error
        printf("Alsa: Error from writei: %s\n", snd_strerror(ret));
		exit(1);
    } else if(ret != (int32)sound_buffer->context->nb_frames) {
		// TODO: Log error/trace
        printf("Alsa: Short write, wrote %d frames\n", ret);
    }
}

void teardown_sound()
{
    snd_pcm_drain(g_linux_context.pcm_device_handle);
    snd_pcm_close(g_linux_context.pcm_device_handle);
}

//uint64 get_timer(void);
//uint64 get_timer_frequency(void);

void test_sound()
{
	  long loops;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  char *buffer;

  /* Open PCM device for playback. */
  rc = snd_pcm_open(&handle, "default",
                    SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_INTERLEAVED);

  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params,
                              SND_PCM_FORMAT_S16_LE);

  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, 2);

  /* 44100 bits/second sampling rate (CD quality) */
  val = 44100;
  snd_pcm_hw_params_set_rate_near(handle, params,
                                  &val, &dir);

  /* Set period size to 32 frames. */
  frames = 32;
  snd_pcm_hw_params_set_period_size_near(handle,
                              params, &frames, &dir);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames,
                                    &dir);
  size = frames * 4; /* 2 bytes/sample, 2 channels */
  buffer = (char *) malloc(size);

  /* We want to loop for 5 seconds */
  snd_pcm_hw_params_get_period_time(params,
                                    &val, &dir);
  /* 5 seconds in microseconds divided by
   * period time */
  loops = 5000000 / val;

  while (loops > 0) {
    loops--;
    rc = read(0, buffer, size);
    if (rc == 0) {
      fprintf(stderr, "end of file on input\n");
      break;
    } else if (rc != size) {
      fprintf(stderr,
              "short read: read %d bytes\n", rc);
    }
    rc = snd_pcm_writei(handle, buffer, frames);
    if (rc == -EPIPE) {
      /* EPIPE means underrun */
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr,
              "error from writei: %s\n",
              snd_strerror(rc));
    }  else if (rc != (int)frames) {
      fprintf(stderr,
              "short write, write %d frames\n", rc);
    }
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  exit(0);
}

int main()
{
	//test_sound();
	//TODO: Maybe use this to identify the sound device to use?

	//int card = -1;
	//const char* iface = "ctl";
	//void** hints;
	//ret = snd_device_name_hint(card, iface, &hints);
	//printf("Ret: %d\n", ret);

	//int i = 0;
	//if(hints == NULL) {
	//	return -1;
	//}
	//while(hints[i] != NULL) {
	//	char* res = snd_device_name_get_hint(hints[i], "NAME");
	//	char* res2 = snd_device_name_get_hint(hints[i], "DESC");
	//	int idx = snd_card_get_index(res);
	//	snd_ctl_t* ctlp;
	//	// TODO: Use ASYNC instead of nonblock?
	//	ret	= snd_ctl_open(&ctlp, res, SND_CTL_ASYNC); 
	//	printf("Ret open card: %d\n", ret);
	//	if(res != NULL) {
	//		printf("Name: %s\nDesc: %s\nIndex: %d\n", res, res2, idx);
	//	}
	//	free(res);
	//	free(res2);
	//	i++;
	//}
	//
 
	game_main();
	return 0;
}
