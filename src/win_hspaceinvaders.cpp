#pragma GCC diagnostic ignored "-Wmissing-field-initializers" 

#include <windows.h>
#include "include/hspaceinvaders.h"
#include <stdio.h> //TODO: Delete after testing done

static bool g_running;
static BITMAPINFO g_bm_info;
static void* g_bitmap;
static HBITMAP g_bitmap_handle;
static HDC g_device_context;

static void win32_resize_DIB_section(const int width, const int height)
{
    // check if bitmap already initialized
    if(g_bitmap_handle != NULL) {
        DeleteObject(g_bitmap_handle);
    }

    if(g_device_context != 0) {
        // DC compatible with the screen (why, windows?)
        g_device_context = CreateCompatibleDC(0);
    }

    g_bm_info = {
        .bmiHeader = {
            .biSize = sizeof(BITMAPINFOHEADER),
            .biWidth = width,
            .biHeight = height,
            .biPlanes = 1,
            .biBitCount = 32,
            .biCompression = BI_RGB
        }
    };
    g_bitmap_handle = CreateDIBSection(
            g_device_context,
            &g_bm_info,
            DIB_RGB_COLORS,
            &g_bitmap,
            NULL,
            0            
    );
}

static void win32_update_window(const int x, const int y, const int width, const int height)
{
    StretchDIBits(
        g_device_context,
        x, y, width, height,
        x, y, width, height,
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
        BeginPaint(window, &paint);
        int x = paint.rcPaint.left;
        int y = paint.rcPaint.top;
        int width = paint.rcPaint.right - paint.rcPaint.left;
        int height = paint.rcPaint.bottom - paint.rcPaint.top;
        win32_update_window(x, y, width, height);
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
    while(g_running) {
        MSG message;
        if(GetMessageA(&message, window, 0, 0) <= 0) {
            break;
        }
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return 0;
}
