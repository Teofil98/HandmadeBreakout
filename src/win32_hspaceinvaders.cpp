#include "include/defines.h"
#include "include/hspaceinvaders.h"
#include "include/platform_layer.h"
#include "include/logging.h"
#include "include/input.h"
#include <objbase.h>
#include <stdio.h> // TODO: Delete after testing done
#include <windows.h>
#include <xaudio2.h>

// TODO: XINPUT handling for controller support

struct platform_backbuffer_context {
    BITMAPINFO bm_info;
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

struct win32_context {
    HINSTANCE program_instance;
    WAVEFORMATEX wave_format;
};

struct platform_window_context {
    HWND window_handle;
    ATOM window_class;
};

struct platform_sound_buffer_context {
    XAUDIO2_BUFFER buffer;
};

// TODO: Global variables for now, see later if I want to change them
static bool g_running;
// TODO: Only needed for WM_PAINT, see if I want to keep it or not
static platform_backbuffer* g_backbuffer; // TODO: See when/if to free this
// TODO: See when/if/how to free stuff inside this
static win32_xaudio2 g_xaudio2;
static win32_context g_context;

// TODO: This is also defined in Linux,
// maybe move in common place
key_state keys[NUM_KEYS];

static win32_window_size win32_get_window_size(const HWND window)
{
    win32_window_size size;
    RECT client_rect;
    GetClientRect(window, &client_rect);
    size.width = client_rect.right - client_rect.left;
    size.height = client_rect.bottom - client_rect.top;
    return size;
}

platform_backbuffer* create_backbuffer(const uint32 width, const uint32 height,
                                       const uint32 bytes_per_pixel)
{
    platform_backbuffer* backbuffer = new platform_backbuffer;
    backbuffer->context = new platform_backbuffer_context;
    backbuffer->width = width;
    backbuffer->height = height;
    backbuffer->bytes_per_pixel = bytes_per_pixel;
    backbuffer->context->bm_info = {
        .bmiHeader = { .biSize = sizeof(BITMAPINFOHEADER),
                       .biWidth = (int32)width,
                       // Negative height to create a top-down DIB
                       .biHeight = -(int32)height,
                       .biPlanes = 1,
                       .biBitCount = 32,
                       .biCompression = BI_RGB }
    };
    backbuffer->bitmap = VirtualAlloc(
        NULL, width * height * backbuffer->bytes_per_pixel,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    // Needed for WM_PAINT
    g_backbuffer = backbuffer;
    // TODO: Maybe clear this to black
    return backbuffer;
}

void destroy_backbuffer(platform_backbuffer* backbuffer)
{
    if(!VirtualFree(backbuffer->bitmap, 0, MEM_RELEASE)) {
        // TODO: Log error
        printf("Failed to free allocated backbuffer\n");
    }
    delete backbuffer->context;
    delete backbuffer;
}

static void win32_display_backbuffer(const platform_backbuffer* backbuffer,
                                     const HDC device_context, const int32 x,
                                     const int32 y, const uint32 window_width,
                                     const uint32 window_height)
{
    // TODO: Correct aspect ratio
    StretchDIBits(device_context, x, y, window_width, window_height, x, y,
                  backbuffer->width, backbuffer->height, backbuffer->bitmap,
                  &backbuffer->context->bm_info, DIB_RGB_COLORS, SRCCOPY);
}

LRESULT win32_window_callback(HWND window, UINT message, WPARAM w_param,
                              LPARAM l_param)
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
        // TODO: Decide if I want to keep WM_PAINT or not, as it needs some ugly
        // global variables
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
        case WM_SYSKEYUP:   // fallthrough
        // TODO: Handle ALT+F4
        case WM_KEYUP: // falthrough
        case WM_KEYDOWN: {
            // FIXME: Implement proper {held, pressed, released, os?} button
            // state once system independent layer is in progress. (HINT:
            // l_param)
            if(w_param == 'W') {
                printf("W\n");
            }
            if(w_param == 'S') {
                printf("S\n");
            }
            if(w_param == VK_ESCAPE) {
                g_running = false;
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

    result = g_xaudio2.xaudio2->CreateMasteringVoice(
        &g_xaudio2.mastering_voice);
    if(FAILED(result)) {
        // TODO: Log and handle error
        printf("Error creating mastering voice\n");
    }

    // TODO: Later on maybe I want creating source voices to be a separate
    // function
    result = g_xaudio2.xaudio2->CreateSourceVoice(&g_xaudio2.source_voice,
                                                  wave_format);
    if(FAILED(result)) {
        // TODO: Log and handle error
        printf("Error creating source voice.\n");
    }
}

platform_window* open_window(const char* title, const uint32 width,
                             const uint32 height)
{
    const WNDCLASSEXA window_class = {
        .cbSize = sizeof(WNDCLASSEXA),
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = win32_window_callback,
        .hInstance = g_context.program_instance,
        .lpszClassName = "HandmadeSpaceInvadersWindowClass"
    };
    ATOM w_class_atom = RegisterClassExA(&window_class);
    if(w_class_atom == 0) {
        // TODO: Log ERROR && Error handling
        printf("Error, could not register window class\n");
        // return -1; // TODO: Find actual code I want to return.
    }

    DWORD window_style = WS_OVERLAPPEDWINDOW;
    // This is the desired windows sized such that the client area has desired
    // height/width
    RECT desired_window_rect = { .left = CW_USEDEFAULT,
                                 .top = CW_USEDEFAULT,
                                 .right = CW_USEDEFAULT + (int32)width,
                                 .bottom = CW_USEDEFAULT + (int32)height };
    win32_window_size desired_window_size;
    AdjustWindowRectEx(&desired_window_rect, window_style, false, 0);
    desired_window_size.height = desired_window_rect.bottom
                                 - desired_window_rect.top;
    desired_window_size.width = desired_window_rect.right
                                - desired_window_rect.left;

    const HWND window = CreateWindowExA(
        0, window_class.lpszClassName, title, window_style | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, desired_window_size.width,
        desired_window_size.height, NULL, NULL, g_context.program_instance,
        NULL);

    if(window == NULL) {
        // TODO: Log ERROR && Error handling
        printf("Error, could not create window");
        // return -1; // TODO: Find actual code I want to return.
    }

    // TODO: Set title
    platform_window* plat_window = new platform_window;
    plat_window->context = new platform_window_context;
    plat_window->context->window_handle = window;
    plat_window->context->window_class = w_class_atom;
    // plat_window->width = width;
    // plat_window->height = height;
    return plat_window;
}

void destroy_window(platform_window* window)
{
    // TODO: Could let windows clean this up
    if(!DestroyWindow(window->context->window_handle)) {
        // TODO: Log and error handling.
        printf("Failed to destroy window\n");
    }
    // TODO: Class automatically unregistered when program terminates
    // See if I want to unregister manually
    delete window->context;
    delete window;
}

void init_sound(const uint16 nb_channels, const uint32 nb_samples_per_sec,
                const uint8 bits_per_sample, const char* name)
{
    const uint16 block_align = (nb_channels * bits_per_sample) / 8;
    const uint32 avg_bytes_per_sec = nb_samples_per_sec * block_align;
    UNUSED(name);
    g_context.wave_format = {
        .wFormatTag = WAVE_FORMAT_PCM, // TODO: See if this is the format I want
                                       // to use
        .nChannels = nb_channels,
        .nSamplesPerSec = nb_samples_per_sec,
        .nAvgBytesPerSec = avg_bytes_per_sec,
        .nBlockAlign = block_align,
        .wBitsPerSample = bits_per_sample,
        .cbSize = 0
    };

    win32_xaudio2_init(&g_context.wave_format);
}

// FIXME: Implement teardown_sound
void teardown_sound()
{
    LOG_TRACE("Tearing down sound subsystem\n");
    // TODO: Implement
    LOG_TRACE("Sound subsystem closed\n");
}

void teardown_input(void)
{
    LOG_TRACE("Tearing down input subsystem\n");
    LOG_TRACE("Input subsystem closed\n");
}

float64 get_time_ms(void)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    
    // Get the frequency of the performance counter
    QueryPerformanceFrequency(&frequency);
    
    // Get the current value of the performance counter
    QueryPerformanceCounter(&counter);
    
    // Convert the counter value to milliseconds
    return (counter.QuadPart * 1000.0) / frequency.QuadPart;
}

// FIXME: Change signature to include size of buffer
platform_sound_buffer* create_sound_buffer(uint32 size_frames)
{
    platform_sound_buffer* sound_buffer = new platform_sound_buffer;
    sound_buffer->context = new platform_sound_buffer_context;

//    const uint32 sound_buffer_size
//        = g_context.wave_format
//              .nAvgBytesPerSec; // TODO: See how much I want this value to be
    // FIXME: Replace 4 with actual computation using knwon values
    const uint32 sound_buffer_size = size_frames * 4;
    void* platform_sound_buffer = VirtualAlloc(
        NULL, sound_buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    sound_buffer->buffer = platform_sound_buffer;
    sound_buffer->bits_per_sample = g_context.wave_format.wBitsPerSample;
    sound_buffer->nb_samples_per_sec = g_context.wave_format.nSamplesPerSec;
    sound_buffer->nb_channels = g_context.wave_format.nChannels;
    sound_buffer->size_bytes = sound_buffer_size;
    sound_buffer->context->buffer = {
        .Flags = 0,
        .AudioBytes = sound_buffer_size,
        .pAudioData = (uint8*)platform_sound_buffer,
        .PlayBegin = 0,
        .PlayLength = 0,
        .LoopBegin = 0,
        .LoopLength = g_context.wave_format.nSamplesPerSec,
        .LoopCount = XAUDIO2_LOOP_INFINITE,
        .pContext = NULL
    };

    return sound_buffer;
}

// TODO: This is currently the same implementation as in Linux, maybe it
// can be implemented in platform layer
void init_input(void)
{
    LOG_TRACE("Initializing input subsystem\n");
    for(int i = 0; i < NUM_KEYS; i++) {
        keys[i].pressed = false;
        keys[i].held = false;
        keys[i].released = false;
    }
    LOG_TRACE("Input subsystem successfully initialized\n");
}

void destroy_sound_buffer(platform_sound_buffer* sound_buffer)
{
    if(!VirtualFree(sound_buffer->buffer, 0, MEM_RELEASE)) {
        // TODO: Log error
        printf("Could not free allocated sound buffer\n");
    }
    delete sound_buffer->context;
    delete sound_buffer;
}

void play_sound_buffer(platform_sound_buffer* sound_buffer)
{
    // TODO: Probably want submitting a buffer and playing to be one or more
    // separate functions
    HRESULT result = g_xaudio2.source_voice->SubmitSourceBuffer(
        &sound_buffer->context->buffer);
    if(FAILED(result)) {
        // TODO: Log and handle error
        printf("Error submitting sound buffer to source voice\n");
    }

    result = g_xaudio2.source_voice->Start(0);
    if(FAILED(result)) {
        // TODO: Log and handle error
        printf("Error playing sound buffer on source voice\n");
    }
}

uint64 get_timer(void)
{
    LARGE_INTEGER measurement;
    QueryPerformanceCounter(&measurement);
    return measurement.QuadPart;
}

uint64 get_timer_frequency(void)
{
    LARGE_INTEGER counter_frequency;
    QueryPerformanceFrequency(&counter_frequency);
    return counter_frequency.QuadPart;
}

void poll_platform_messages(void)
{
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
}

void display_backbuffer(const platform_backbuffer* backbuffer,
                        const platform_window* window)
{
    const HDC hdc = GetDC(window->context->window_handle);
    win32_window_size window_size = win32_get_window_size(
        window->context->window_handle);
    StretchDIBits(hdc, 0, 0, window_size.width, window_size.height, 0, 0,
                  backbuffer->width, backbuffer->height, backbuffer->bitmap,
                  &backbuffer->context->bm_info, DIB_RGB_COLORS, SRCCOPY);
    // TODO: See if I can/should use OWNDC and use the same DC without releasing
    ReleaseDC(window->context->window_handle, hdc);
}

bool should_close() { return !g_running; }

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance,
                     LPSTR cmd_line, int show_cmd)
{
    UNUSED(prev_instance);
    UNUSED(cmd_line);
    UNUSED(show_cmd);

    // Set up the global context for the win32 platform
    g_context.program_instance = instance;
    g_running = true;

    game_main();

    return 0;
}
