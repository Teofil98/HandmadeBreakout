#include <X11/Xlib.h>
#include <stdio.h>
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

    Window window = XCreateSimpleWindow(g_display_server, g_root_window, 
                    POSX, POSY, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H, BORDER_W, 
                    BlackPixel(g_display_server, g_screen), WhitePixel(g_display_server, g_screen));

    // Make window visible
    XMapWindow(g_display_server, window);

    XEvent event;
    while(XNextEvent(g_display_server, &event)) {}

    XUnmapWindow(g_display_server, window);
    XDestroyWindow(g_display_server, window);
    XCloseDisplay(g_display_server);
    return 0;
}
