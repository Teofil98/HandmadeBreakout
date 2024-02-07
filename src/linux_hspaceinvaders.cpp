#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/XKBlib.h>
#include "include/defines.h"

static Display* g_display_server;
static uint64 g_screen;
static Window g_root_window;

// initial window position
// TODO: See if I want to add something like this in windos
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

    XSetWindowAttributes attributes;
    attributes.background_pixel = WhitePixel(g_display_server, g_screen);
    attributes.border_pixel = BlackPixel(g_display_server, g_screen);
    attributes.event_mask = KeyReleaseMask;

    Window window = XCreateWindow(g_display_server, g_root_window, 
                    POSX, POSY, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, BORDER_W, 
                    DefaultDepth(g_display_server, g_screen), InputOutput, DefaultVisual(g_display_server, g_screen),
                    CWBackPixel | CWBorderPixel | CWEventMask, &attributes);

    // Make window visible
    XMapWindow(g_display_server, window);

    XEvent event;
    // TODO: Don't want the loop to be here, but rather in game layer
    // This is just for testing
    bool exit = false;
    // TODO: XNextEvent is blocking :( Look into XCheckMaskEvent
    while( !exit && (XNextEvent(g_display_server, &event) == 0))  
    {
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
                    //printf("Converted esc: %d\n", XKeysymToKeycode(g_display_server, XK_W));
                }
            } break;
            default: printf("Event type: %d\n", event.xkey.type);
        }
    }
    XUnmapWindow(g_display_server, window);
    XDestroyWindow(g_display_server, window);
    XCloseDisplay(g_display_server);
    return 0;
}
