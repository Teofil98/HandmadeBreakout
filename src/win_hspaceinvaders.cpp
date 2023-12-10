#pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
#pragma GCC diagnostic warning "-Wunused-function"
#pragma GCC diagnostic warning "-Wunused-variable"

#include <windows.h>
#include "include/hspaceinvaders.h"
#include <stdio.h> //TODO: Delete after testing done
#include <stdint.h>

#define RGBA_ALPHA(x) ((uint8_t)x << 24)
#define RGBA_RED(x) ((uint8_t)x << 16)
#define RGBA_GREEN(x) ((uint8_t)x << 8)
#define RGBA_BLUE(x) ((uint8_t)x << 0)
#define RGBA(r, g, b, a) (RGBA_RED(r) | RGBA_GREEN(g) | RGBA_BLUE(b) | RGBA_ALPHA(a))

static bool g_running;
static BITMAPINFO g_bm_info;
static void* g_bitmap;
static int g_bitmap_width;
static int g_bitmap_height;

static void win32_resize_DIB_section(const int width, const int height)
{
    if(g_bitmap != NULL) {
        VirtualFree(g_bitmap, 0, MEM_RELEASE);
        g_bitmap = NULL;
    }
    g_bitmap_width = width;
    g_bitmap_height = height;
    g_bm_info = {
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
    g_bitmap = VirtualAlloc(
                    NULL, width * height * 4, 
                    MEM_COMMIT, PAGE_READWRITE
                ); 
    //TODO: Maybe clear this to black
}

static void draw_gradient(int row_offset, int col_offset)
{
    uint32_t* pixels = (uint32_t*)g_bitmap;
    const int nb_rows = g_bitmap_height;
    const int nb_cols = g_bitmap_width;
    for(int row = 0; row < nb_rows; row ++) {
        for(int col = 0; col < nb_cols; col ++) {
            pixels[row * nb_cols + col] = 
                    RGBA(0, (uint8_t)(row + row_offset), 
                    (uint8_t)(col + col_offset), 0); // TODO: Add RGBA macros 
        }
    }
}

static void win32_update_window(HDC device_context, const int x, const int y,
                const int width, const int height)
{
    // TODO: Check if src and dest are correct
    StretchDIBits(
        device_context,
        x, y, width, height,
        x, y, g_bitmap_width, g_bitmap_height,
        g_bitmap, &g_bm_info,
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
        RECT client_rect;
        GetClientRect(window, &client_rect); // TODO: Handle GetClientRect failure? 
        int width = client_rect.right - client_rect.left;
        int height = client_rect.bottom - client_rect.top;
        win32_resize_DIB_section(width, height);
    } break;
    case WM_CLOSE: {
        // TODO: display message to user before closing?
        // TODO: Handle destroy window somewhere, even though windows 
        // destroys it on app exit.
        g_running = false;
    } break;
    case WM_DESTROY: {
        //TODO: Treat this as error and recreate window?
        g_running = false;
    } break;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC device_context = BeginPaint(window, &paint);
        int x = paint.rcPaint.left;
        int y = paint.rcPaint.top;
        int width = paint.rcPaint.right - paint.rcPaint.left;
        int height = paint.rcPaint.bottom - paint.rcPaint.top;
        draw_gradient(0, 0);
        // FIXME: Check how we can redraw using the PAINTSTRUCT
        // instead of the whole screen
        //win32_update_window(device_context, x, y, width, height);
        RECT client_rect;
        GetClientRect(window, &client_rect);
        width = client_rect.right - client_rect.left;
        height = client_rect.bottom - client_rect.top;
        win32_update_window(device_context, 0, 0, width, height);
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
    // Define window class
    WNDCLASSEXA window_class = {
        .cbSize = sizeof(WNDCLASSEXA),
        .lpfnWndProc = win32_window_callback,
        .hInstance = instance,
        .lpszClassName = "HandmadeSpaceInvadersWindowClass"
    };

    if(RegisterClassExA(&window_class) == 0) {
        //TODO: Log ERROR && Error handling
        return -1; //TODO: Find actual code I want to return. 
    }

    HWND window = CreateWindowExA(
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
        //TODO: Log ERROR && Error handling
        return -1; //TODO: Find actual code I want to return. 
    }

    g_running = 1;
    int xoffset = 0, yoffset = 0;
    while(g_running) {
        MSG message;
        while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            if(message.message == WM_QUIT) {
                g_running = false;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        draw_gradient(xoffset, yoffset++);
        HDC hdc = GetDC(window);
        win32_update_window(hdc, 0, 0, g_bitmap_width, g_bitmap_height);
        ReleaseDC(window, hdc);
    }

    return 0;
}
