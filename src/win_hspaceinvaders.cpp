#pragma GCC diagnostic ignored "-Wmissing-field-initializers" 

#include <windows.h>
#include "include/hspaceinvaders.h"

LRESULT window_callback(
  HWND window,
  UINT message,
  WPARAM w_param,
  LPARAM l_param 
)
{
   UNUSED(w_param); 
   UNUSED(l_param); 
   return DefWindowProcA(window, message, w_param, l_param);
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
        .lpfnWndProc = window_callback,
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

    while(1) {      
        MSG message;
        if(GetMessageA(&message, window, 0, 0) == false) {
            PostQuitMessage(0);
            break;
        }
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return 0;
}
