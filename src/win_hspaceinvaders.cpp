#pragma GCC diagnostic ignored "-Wmissing-field-initializers" 
#pragma GCC diagnostic warning "-Wunused-function"
#pragma GCC diagnostic warning "-Wunused-variable"
#pragma GCC diagnostic warning "-Wunused-but-set-variable"

#include <windows.h>
#include <objbase.h>
#include <xaudio2.h>
#include "include/hspaceinvaders.h"
#include <stdio.h> // TODO: Delete after testing done
#include <stdint.h>
#include <math.h> // TODO: replace functions here with own implementation

#define RGBA_ALPHA(x) ((uint8_t)x << 24)
#define RGBA_RED(x) ((uint8_t)x << 16)
#define RGBA_GREEN(x) ((uint8_t)x << 8)
#define RGBA_BLUE(x) ((uint8_t)x << 0)
#define RGBA(r, g, b, a) (RGBA_RED(r) | RGBA_GREEN(g) | RGBA_BLUE(b) | RGBA_ALPHA(a))

#define DEFAULT_WINDOW_W 1280 
#define DEFAULT_WINDOW_H 720

#define PI 3.14159265359

using uint64  = uint64_t;
using uint32  = uint32_t;
using uint16  = uint16_t;
using uint8   = uint8_t;
using int64   = int64_t;
using int32   = int32_t;
using int16   = int16_t;
using int8    = int8_t;
using float32 = float;
using float64 = double;

// TODO: XINPUT handling for controller support

struct win32_backbuffer {
    BITMAPINFO bm_info;
    uint32 bytes_per_pixel;
    void* bitmap;
    uint32 width;
    uint32 height;
};

struct win32_window_size {
    uint32 width;
    uint32 height;
};

struct win32_xaudio2 {
    IXAudio2* xaudio2;
    IXAudio2SourceVoice* source_voice; 
    IXAudio2MasteringVoice* mastering_voice;
};

// TODO: Global variables for now, see later if I want to change them
static bool g_running;
static win32_backbuffer g_backbuffer; // TODO: See when/if to free this
static win32_xaudio2 g_xaudio2; // TODO: See when/if/how to free stuff inside this

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
                    const int32 width, const int32 height)
{
    if(backbuffer.bitmap != NULL) {
        // FIXME: With fixed buffer, this is no longer called
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
                    MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE
                ); 
    // TODO: Maybe clear this to black
}

static void draw_gradient(const win32_backbuffer& backbuffer, 
                const uint32 row_offset, const uint32 col_offset)
{
    uint32_t* pixels = (uint32_t*)backbuffer.bitmap;
    const uint32 nb_rows = backbuffer.height;
    const uint32 nb_cols = backbuffer.width;
    for(uint32 row = 0; row < nb_rows; row ++) {
        for(uint32 col = 0; col < nb_cols; col ++) {
            pixels[row * nb_cols + col] = 
                    RGBA(0, (uint8)(row + row_offset), 
                    (uint8)(col + col_offset), 0);
        }
    }
}

static void win32_display_backbuffer(const win32_backbuffer& backbuffer, 
                    const HDC device_context,
                    const int32 x, const int32 y, 
                    const uint32 window_width, const uint32 window_height)
{
    // TODO: Correct aspect ratio
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
        win32_window_size window_size = win32_get_window_size(window);
        // update whole window. No need to make things more complicated
        // by updating only dirty part
        win32_display_backbuffer(g_backbuffer, device_context, 0, 0, 
                        window_size.width, window_size.height);
        EndPaint(window, &paint);
    } break;
    case WM_SYSKEYDOWN: // fallthrough
    case WM_SYSKEYUP: // fallthrough
    // TODO: Handle ALT+F4
    case WM_KEYUP: // falthrough
    case WM_KEYDOWN: {
        // FIXME: Implement proper {held, pressed, released, os?} button state once 
        // system independent layer is in progress. (HINT: l_param) 
        if(w_param == 'W') {
            printf("W\n");
        }
        if(w_param == 'S') {
            printf("S\n");
        }
    } break;
    default: {
        result = DefWindowProcA(window, message, w_param, l_param);
    } 
    } // end switch
    return result; 
}

void win32_xaudio2_init(const WAVEFORMATEX* wave_format)
{
    // initialize COM
    HRESULT result = CoInitializeEx(0, COINIT_MULTITHREADED);
    if(FAILED(result)) {
        // TODO: Log and handle error
        printf("Error initializing COM\n");
    }

    result = XAudio2Create(&g_xaudio2.xaudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if(FAILED(result)) {
        // TODO: Log and handle error
        printf("Error initializing xaudio2\n");
    }

    result = g_xaudio2.xaudio2->CreateMasteringVoice(&g_xaudio2.mastering_voice);
    if(FAILED(result)) {
        // TODO: Log and handle error
        printf("Error creating mastering voice\n");
    }

    // TODO: Later on maybe I want creating source voices to be a separate function
    result = g_xaudio2.xaudio2->CreateSourceVoice(&g_xaudio2.source_voice, wave_format);
    if(FAILED(result)) {
        // TODO: Log and handle error
        printf("Error creating source voice.\n");
    }
}

// FIXME: Certain frequencies produce audible skip
void win32_write_square_wave(XAUDIO2_BUFFER* xaudio2_buffer, const uint32 frequency, const int tone_volume)
{
    // TODO: Consider if I want to have non 16b/sample  audio
    // TODO: xaudio2_buffer->NbBytes should be multiple of 2, maybe assert
    uint16* buffer = (uint16*)xaudio2_buffer->pAudioData;
    int32 nb_samples = xaudio2_buffer->AudioBytes/2;
    // TODO:  For now, I assume that the buffer lasts for 1 second
    // FIXME: Deal with buffers that have length more than 1 sec
    const uint32 square_wave_period = nb_samples / frequency;
    const uint32 half_period = square_wave_period / 2; 

    for(int i = 0; i < nb_samples; i += 2)
    {
        int sign = (i / half_period) % 2 == 0 ? 1 : -1;
        // set left and right samples
        buffer[i] = sign * tone_volume;
        buffer[i + 1] = sign * tone_volume;
    }
}

void win32_write_sin_wave(XAUDIO2_BUFFER* xaudio2_buffer, const uint32 frequency, const int tone_volume)
{
    // TODO: Consider if I want to have non 16b/sample  audio
    // TODO: xaudio2_buffer->NbBytes should be multiple of 2, maybe assert
    uint16* buffer = (uint16*)xaudio2_buffer->pAudioData;
    int32 nb_samples = xaudio2_buffer->AudioBytes/2;
    // TODO:  For now, I assume that the buffer lasts for 1 second
    // FIXME: Deal with buffers that have length more than 1 sec
    const uint32 wave_period = nb_samples / frequency;

    for(int i = 0; i < nb_samples; i += 2)
    {
        // Where in the sin wave we are, in radians
        float32 sin_location = 2 * PI * ((i % wave_period) / (float32) wave_period); 
        float32 sin_value = sinf(sin_location);
        // set left and right samples
        buffer[i] = (uint16) (sin_value * tone_volume);
        buffer[i + 1] = (uint16) (sin_value * tone_volume);
    }
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

    DWORD window_style = WS_OVERLAPPEDWINDOW;
    // This is the desired windows sized such that the client area has desired height/width
    RECT desired_window_rect = {
        .left = CW_USEDEFAULT,
        .top = CW_USEDEFAULT,
        .right = CW_USEDEFAULT + DEFAULT_WINDOW_W,
        .bottom = CW_USEDEFAULT + DEFAULT_WINDOW_H
    };
    win32_window_size desired_window_size;
    AdjustWindowRectEx(&desired_window_rect, window_style, false, 0);
    desired_window_size.height = desired_window_rect.bottom - desired_window_rect.top;
    desired_window_size.width = desired_window_rect.right - desired_window_rect.left;

    const HWND window = CreateWindowExA(
                        0, 
                        window_class.lpszClassName,
                        "Handmade Space Invaders", 
                        window_style | WS_VISIBLE, 
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        desired_window_size.width,
                        desired_window_size.height,
                        NULL,
                        NULL,
                        instance,
                        NULL
                    );

    if(window == NULL) {
        // TODO: Log ERROR && Error handling
        return -1; // TODO: Find actual code I want to return. 
    }
    win32_window_size window_size = win32_get_window_size(window);
    win32_resize_DIB_section(g_backbuffer, DEFAULT_WINDOW_W, DEFAULT_WINDOW_H);
    int32 xoffset = 0, yoffset = 0; // Used for gradient animation

    // Sound initialization
    const int8 nb_channels = 2;
    const int32 nb_samples_per_sec = 44100;
    const int8 bits_per_sample = 16;
    const int32 block_align = (nb_channels * bits_per_sample)/8;
    const int32 avg_bytes_per_sec = nb_samples_per_sec * block_align;
    WAVEFORMATEX wave_format = {
        .wFormatTag = WAVE_FORMAT_PCM, // TODO: See if this is the format I want to use
        .nChannels = nb_channels,
        .nSamplesPerSec = nb_samples_per_sec,
        .nAvgBytesPerSec = avg_bytes_per_sec,
        .nBlockAlign = block_align,
        .wBitsPerSample = bits_per_sample,
        .cbSize = 0        
    };

    win32_xaudio2_init(&wave_format);
    const int32 sound_buffer_size = avg_bytes_per_sec; // TODO: See how much I want this value to be
    XAUDIO2_BUFFER sound_buffer = {
        .Flags = 0,
        .AudioBytes = sound_buffer_size, 
        .pAudioData =  (uint8*) VirtualAlloc(
                    NULL, sound_buffer_size, 
                    MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE
                ),
        .PlayBegin = 0,
        .PlayLength = 0,
        .LoopBegin = 0,
        .LoopLength = nb_samples_per_sec,
        .LoopCount = XAUDIO2_LOOP_INFINITE,
        .pContext = NULL
    };

    const uint32 frequency = 440;
    const int32 tone_volume = 2200;
    //win32_write_square_wave(&sound_buffer, frequency, tone_volume);
    win32_write_sin_wave(&sound_buffer, frequency, tone_volume);
    // TODO: Probably want submitting a buffer and playing to be one or more separate functions
    HRESULT result = g_xaudio2.source_voice->SubmitSourceBuffer(&sound_buffer);
    if(FAILED(result)) {
        // TODO: Log and handle error
        printf("Error submitting sound buffer to source voice\n");
    }

    result = g_xaudio2.source_voice->Start(0);
    if(FAILED(result)) {
        // TODO: Log and handle error
        printf("Error playing sound buffer on source voice\n");
    }

    // Processing loop
    g_running = true;
    while(g_running) {
        MSG message;
        // TODO: Do I want to do a while here?
        while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            // FIXME: Can probably do this check in the switch in the callbacK
            if(message.message == WM_QUIT) {
                g_running = false;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        draw_gradient(g_backbuffer, xoffset, yoffset++);
        const HDC hdc = GetDC(window);
        window_size = win32_get_window_size(window);
        win32_display_backbuffer(g_backbuffer, hdc, 0, 0, window_size.width, window_size.height);
        // TODO: See if I can/should use OWNDC and use the same DC without releasing
        ReleaseDC(window, hdc);
    }

    return 0;
}
