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

static Display* g_display_server;
static uint64 g_screen;
static Window g_root_window;

// initial window position
// TODO: See if I want to add something like this in windows
// Also see if I actually need them here
#define POSX 500
#define POSY 500
#define BORDER_W 15

int main()
{
    g_display_server = XOpenDisplay(NULL);
    if(!g_display_server) {
        // TODO: Log error
        printf("Error, can't open display\n");
    }

    g_screen = DefaultScreen(g_display_server);
    g_root_window = RootWindow(g_display_server, g_screen);

	// create image buffer which will be used to blit to screen
	// TODO: Change from DefaultDepth to something which includes the alpha channel as well
	// FIXME: Why is this working with 4 if by default I only have 24 bits???? (Could be the bitmap_pad)
	// FIXME: Figure out how to make a window with depth of 32
	
	// NOTE: bits_per_pixel = 32 already, I guess the 8 bits get ignored by default for the 24 depth window??
	int PIXEL_SIZE = 4;
	char* backbuffer = (char*) malloc(PIXEL_SIZE * DEFAULT_WINDOW_H * DEFAULT_WINDOW_W); 
	XImage* image = XCreateImage(g_display_server, DefaultVisual(g_display_server, g_screen),
						 DefaultDepth(g_display_server, g_screen), ZPixmap,
						 0, backbuffer, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H,
						 32, // TODO: probably want to change this bitmap_pad to 32 once I get the depth sorted out
						 0);


	printf("Bitmap unit: %d\nBits per pixel: %d\n\n", image->bitmap_unit, image->bits_per_pixel);
	printf("Direct Color: %d\n", DirectColor);

	for(int i = 0; i < DEFAULT_WINDOW_H; i++) {
		for(int j = 0; j < DEFAULT_WINDOW_W * PIXEL_SIZE; j += PIXEL_SIZE) {
			backbuffer[i * DEFAULT_WINDOW_W * PIXEL_SIZE + j + 1] = 120;
		}
	}


	printf("Default depth: %d\n",DefaultDepth(g_display_server, g_screen)); 

    XSetWindowAttributes attributes;
    attributes.background_pixel = WhitePixel(g_display_server, g_screen);
    attributes.border_pixel = BlackPixel(g_display_server, g_screen);
    attributes.event_mask = KeyReleaseMask;

	// TODO: Change from DefaultDepth to something which includes the alpha channel as well
    Window window = XCreateWindow(g_display_server, g_root_window, 
                    POSX, POSY, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, BORDER_W, 
                    DefaultDepth(g_display_server, g_screen), InputOutput, DefaultVisual(g_display_server, g_screen),
                    CWBackPixel | CWBorderPixel | CWEventMask, &attributes);

	//Pixmap p = XCreatePixmap(g_display_server, XDefaultRootWindow(g_display_server), 
//			   DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, 32);
	GC gc = XCreateGC(g_display_server, window, 0, NULL);

	XTextProperty window_title_propery;
	// FIXME: Replace this ugly thing with the define called from main game loop
	// For now leave it like this for testing.
	char* window_title = (char*) "Handmade Space Invaders";
	if(XStringListToTextProperty(&window_title, 1, &window_title_propery) == 0) {
		// TODO: handle error
		printf("Error creating window title property\n");
	}
	XSetWMName(g_display_server, window, &window_title_propery);
	// TODO: See if I want a different name for icon title 
	XSetWMIconName(g_display_server, window, &window_title_propery);
    // Make window visible
    XMapWindow(g_display_server, window);

    // TODO: Don't want the loop to be here, but rather in game layer
    // This is just for testing
    bool exit = false;

    while(!exit)  
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
    	                    exit = true; 
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

		XPutImage(g_display_server, window, gc, image, 
				0, 0, 0, 0,
				DEFAULT_WINDOW_W, DEFAULT_WINDOW_H);
    }
	// TODO: CHeck if XCreateImage and XPutImage let me efficiently blit to screen
    XUnmapWindow(g_display_server, window);
    XDestroyWindow(g_display_server, window);
    XCloseDisplay(g_display_server);
    return 0;
}
