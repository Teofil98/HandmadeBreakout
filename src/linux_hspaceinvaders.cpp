#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XKB.h>
#include <cstdlib>
#include <stdio.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#include "include/defines.h"
#include "include/platform_layer.h"
#include "include/hspaceinvaders.h"

#include <string.h> //TODO: Delete later

static Display* g_display_server;
static uint64 g_screen;
static Window g_root_window;
bool g_exit = false;

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

platform_window* open_window(const char* title, const uint32 width, const uint32 height)
{
	g_display_server = XOpenDisplay(NULL);
    if(!g_display_server) {
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


void poll_platform_messages(void)
{
	XEvent event;
	// TODO: Figure out what KeymapStateMask does and if I need it in addition to KeyReleaseMask
	long event_mask =  KeyReleaseMask;
	if(XCheckMaskEvent(g_display_server, event_mask, &event)) {
		// TODO: Figure out what last 2 parameters do (group and level)
		KeySym key = XkbKeycodeToKeysym(g_display_server, event.xkey.keycode, 0, 0);
		switch (event.type) {
			case KeyRelease: {
				switch (key) {
    				case XK_Escape: {
    	    			g_exit = true; 
    	    			} break; 
    	    		case XK_w: {
						printf("W\n");
						} break;
					case XK_s: {
						printf("S\n");
    	    		} break;
					default: printf("Event key type: %lx\n", key); 
    			}
			} break;
			default: printf("Event type: %d\n", event.xkey.type);
    	}
	}
}

int main()
{
	game_main();
	return 0;
}
