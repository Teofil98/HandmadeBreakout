#pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
#pragma GCC diagnostic warning "-Wunused-function"
#pragma GCC diagnostic warning "-Wunused-variable"
#pragma GCC diagnostic warning "-Wunused-but-set-variable"

#include <windows.h>
#include "include/hspaceinvaders.h"
#include <stdio.h> // TODO: Delete after testing done
#include <stdint.h>

#define RGBA_ALPHA(x) ((uint8_t)x << 24)
#define RGBA_RED(x) ((uint8_t)x << 16)
#define RGBA_GREEN(x) ((uint8_t)x << 8)
#define RGBA_BLUE(x) ((uint8_t)x << 0)
#define RGBA(r, g, b, a) (RGBA_RED(r) | RGBA_GREEN(g) | RGBA_BLUE(b) | RGBA_ALPHA(a))

struct win32_backbuffer {
    BITMAPINFO bm_info;
    int bytes_per_pixel;
    void* bitmap;
    int width;
    int height;
};

struct win32_window_size {
    int width;
    int height;
};

// TODO: Global variables for now, see later if I want to change them
static bool g_running;
static win32_backbuffer g_backbuffer;

static win32_window_size win32_get_window_size(const HWND window)
{
    win32_window_size size;
    RECT client_rect;
    GetClientRect(window, &client_rect);
    size.width = client_rect.right - client_rect.left;
    size.height = client_rect.bottom - client_rect.top;
    return size;
}

static void win32_resize_DIB_section(win32_backbuffer& backbuffer, 
                    const int width, const int height)
{
    if(backbuffer.bitmap != NULL) {
        VirtualFree(backbuffer.bitmap, 0, MEM_RELEASE);
        backbuffer.bitmap = NULL;
    }
    backbuffer.width = width;
    backbuffer.height = height;
    backbuffer.bytes_per_pixel = 4;
    backbuffer.bm_info = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = width,
            // Negative height to create a top-down DIB
            .biHeight = -height,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB
        }
    };
    backbuffer.bitmap = VirtualAlloc(
                    NULL, width * height * backbuffer.bytes_per_pixel, 
                    MEM_COMMIT, PAGE_READWRITE
                ); 
    // TODO: Maybe clear this to black
}

static void draw_gradient(const win32_backbuffer& backbuffer, 
                const int row_offset, const int col_offset)
{
    uint32_t* pixels = (uint32_t*)backbuffer.bitmap;
    const int nb_rows = backbuffer.height;
    const int nb_cols = backbuffer.width;
    for(int row = 0; row < nb_rows; row ++) {
        for(int col = 0; col < nb_cols; col ++) {
            pixels[row * nb_cols + col] = 
                    RGBA(0, (uint8_t)(row + row_offset), 
                    (uint8_t)(col + col_offset), 0);
        }
    }
}

static void win32_display_backbuffer(const win32_backbuffer& backbuffer, 
                    const HDC device_context,
                    const int x, const int y, 
                    const int window_width, const int window_height)
{
    // TODO: Check if src and dest are correct
    StretchDIBits(
        device_context,
        x, y, window_width, window_height,
        x, y, backbuffer.width, backbuffer.height,
        backbuffer.bitmap, &backbuffer.bm_info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

LRESULT win32_window_callback(
  HWND window,
  UINT message,
  WPARAM w_param,
  LPARAM l_param 
)
{
    LRESULT result = 0;
    switch(message) {
    case WM_SIZE: {
        win32_window_size window_size = win32_get_window_size(window);
        win32_resize_DIB_section(g_backbuffer, window_size.width, window_size.height);
    } break;
    case WM_CLOSE: {
        // TODO: display message to user before closing?
        // TODO: Handle destroy window somewhere, even though windows 
        // destroys it on app exit.
        g_running = false;
    } break;
    case WM_DESTROY: {
        // TODO: Treat this as error and recreate window?
        g_running = false;
    } break;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC device_context = BeginPaint(window, &paint);
        const int x = paint.rcPaint.left;
        const int y = paint.rcPaint.top;
        const int width = paint.rcPaint.right - paint.rcPaint.left;
        const int height = paint.rcPaint.bottom - paint.rcPaint.top;
        win32_window_size window_size = win32_get_window_size(window);
        // TODO: See if I want to just pass paint width/height or full window width/heighy 
        //win32_display_backbuffer(g_backbuffer, device_context, x, y, width, height);
        win32_display_backbuffer(g_backbuffer, device_context, 0, 0, 
                        window_size.width, window_size.height);
        EndPaint(window, &paint);
    } break;
    default: {
        result = DefWindowProcA(window, message, w_param, l_param);
    } 
    } // end switch
    return result; 
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE prev_instance,
    LPSTR     cmd_line,
    int       show_cmd
)
{
    UNUSED(prev_instance);
    UNUSED(cmd_line);
    UNUSED(show_cmd);
    
    const WNDCLASSEXA window_class = {
        .cbSize = sizeof(WNDCLASSEXA),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = win32_window_callback,
        .hInstance = instance,
        .lpszClassName = "HandmadeSpaceInvadersWindowClass"
    };

    if(RegisterClassExA(&window_class) == 0) {
        // TODO: Log ERROR && Error handling
        return -1; // TODO: Find actual code I want to return. 
    }

    const HWND window = CreateWindowExA(
                        0, 
                        window_class.lpszClassName,
                        "Handmade Space Invaders", 
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        NULL,
                        NULL,
                        instance,
                        NULL
                    );

    if(window == NULL) {
        // TODO: Log ERROR && Error handling
        return -1; // TODO: Find actual code I want to return. 
    }

    int xoffset = 0, yoffset = 0; // Used for gradient animation
    // Processing loop
    g_running = true;
    while(g_running) {
        MSG message;
        while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            if(message.message == WM_QUIT) {
                g_running = false;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        draw_gradient(g_backbuffer, xoffset, yoffset++);
        const HDC hdc = GetDC(window);
        win32_display_backbuffer(g_backbuffer, hdc, 0, 0, g_backbuffer.width, g_backbuffer.height);
        ReleaseDC(window, hdc);
    }

    return 0;
}
