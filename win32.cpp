#define PI 3.14159265359f

#include "Draw.h"
#include "windows.h"
#include "Game.cpp"
#include "Keyboard.h"
#include "win32.h"
#include "Array.h"
#include "String.h"
#include "Win32_Mixer.h"
#include "Wav_File.cpp"

#include <xinput.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>

Sound_Manager sound_manager = {};

// "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

const u16 WINDOW_WIDTH  = 960;
const u16 WINDOW_HEIGHT = 540;

static s32 X_OFFSET = 50;
static s32 Y_OFFSET = 40;

static bool should_quit = false;
static Back_Buffer back_buffer;

static s64 perf_count_freq;

WINDOWPLACEMENT window_replacement;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(xinput_get_state_stub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_get_state *XInputGetState_ = xinput_get_state_stub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(xinput_set_state_stub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_set_state *XInputSetState_ = xinput_set_state_stub;
#define XInputSetState XInputSetState_

void load_xinput() {
    HMODULE xinput_handle = LoadLibraryA("xinput1_3.dll");
    if (!xinput_handle) { return; }

    XInputGetState = (x_input_get_state *)GetProcAddress(xinput_handle, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(xinput_handle, "XInputSetState");
}

typedef BOOL WINAPI set_process_dpi_aware(void);
typedef BOOL WINAPI set_process_dpi_awareness_context(DPI_AWARENESS_CONTEXT);
static void PreventWindowsDPIScaling() {
    HMODULE WinUser = LoadLibraryW(L"user32.dll");
    set_process_dpi_awareness_context *SetProcessDPIAwarenessContext = (set_process_dpi_awareness_context *)GetProcAddress(WinUser, "SetProcessDPIAwarenessContext");
    if (SetProcessDPIAwarenessContext) {
        SetProcessDPIAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    } else {
        set_process_dpi_aware *SetProcessDPIAware = (set_process_dpi_aware *)GetProcAddress(WinUser, "SetProcessDPIAware");
        if (SetProcessDPIAware) {
            SetProcessDPIAware();
        }
    }
}

void toggle_full_screen(HWND window_handle) {
    window_replacement.length = sizeof(WINDOWPLACEMENT);

    DWORD style = GetWindowLong(window_handle, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW) {
        MONITORINFO monitor_info = {};
        monitor_info.cbSize = sizeof(monitor_info);

        if (GetWindowPlacement(window_handle, &window_replacement) &&
            GetMonitorInfo(MonitorFromWindow(window_handle, MONITOR_DEFAULTTOPRIMARY), &monitor_info)) {
            SetWindowLong(window_handle, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window_handle,  HWND_TOP, monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right  - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    } else {
        SetWindowLong(window_handle, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window_handle, &window_replacement);
        SetWindowPos(window_handle, null, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

// Expects 32 bits per pixel bitmap images.
bool load_bitmap_file(char *file_name, Bitmap *bitmap) {
    s64 size = read_file(file_name, (void **)&bitmap->data);
    if (size < 0) { return false; }

    BMP_Header *bmp_header = (BMP_Header *)bitmap->data;
    bitmap->data = (u8 *)bmp_header + bmp_header->bitmap_offset;

    u32 red_mask   = bmp_header->red_mask;
    u32 green_mask = bmp_header->green_mask;
    u32 blue_mask  = bmp_header->blue_mask;
    u32 alpha_mask = bmp_header->alpha_mask;

    u32 red_index;
    u32 green_index;
    u32 blue_index;
    u32 alpha_index;

    assert(bsf(&red_index, red_mask));
    assert(bsf(&green_index, green_mask));
    assert(bsf(&blue_index, blue_mask));
    assert(bsf(&alpha_index, alpha_mask));

    u32 red_shift   = red_index;
    u32 green_shift = green_index;
    u32 blue_shift  = blue_index;
    u32 alpha_shift = alpha_index;

    u32 *source_dest = (u32 *)bitmap->data;
    for (s32 y = 0; y < bmp_header->height; ++y) {
        for (s32 x = 0; x < bmp_header->width; ++x) {
            u32 c = *source_dest;
            *source_dest++ = ((((c >> alpha_shift) & 0xFF) << 24) |
                              (((c >> red_shift)   & 0xFF) << 16) |
                              (((c >> green_shift) & 0xFF) << 8)  |
                              (((c >> blue_shift)  & 0xFF) << 0));

        }
    }

    bitmap->file_name = file_name;
    bitmap->width     = bmp_header->width;
    bitmap->height    = bmp_header->height;

    return true;
}

Window_Dimension get_window_dimension(HWND window_handle) {
    Window_Dimension window_dimension;
    RECT client_rect;
    get_client_rect(window_handle, &client_rect);
    window_dimension.width  = client_rect.right  - client_rect.left;
    window_dimension.height = client_rect.bottom - client_rect.top;
    return window_dimension;
}

void create_backbuffer(Back_Buffer *back_buffer, s32 width, s32 height) {
    if (back_buffer->memory) { VirtualFree(back_buffer->memory, null, MEM_RELEASE); }

    back_buffer->width  = width;
    back_buffer->height = height;

    back_buffer->info.bmiHeader.biSize        = sizeof(back_buffer->info.bmiHeader);
    back_buffer->info.bmiHeader.biWidth       = back_buffer->width;
    back_buffer->info.bmiHeader.biHeight      = -back_buffer->height; // We want a top-down bitmap (starts top left to bottom right)
    back_buffer->info.bmiHeader.biPlanes      = 1;
    back_buffer->info.bmiHeader.biBitCount    = 32;
    back_buffer->info.bmiHeader.biCompression = BI_RGB;

    back_buffer->bpp = 4;
    u32 bitmap_memory_size = back_buffer->bpp * back_buffer->width * back_buffer->height;

    back_buffer->memory = VirtualAlloc(null, bitmap_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    back_buffer->pitch = back_buffer->width * back_buffer->bpp;
}

// Projection of rasterization to window
void swap_buffers(Back_Buffer *buffer, HDC device_context, s32 window_width, s32 window_height) {
    // @Todo: This needs to take into consideration the aspect ratio of the
    // window because when we call stretchDIBs we are distorting the image.
    // @Cleanup: Use BitBlt for speed reasons.
    stretch_dib_bits(device_context,
                     0, 0, window_width,  window_height,
                     0, 0, buffer->width, buffer->height,
                     buffer->memory,
                     &buffer->info,
                     DIB_RGB_COLORS, SRCCOPY);

    // @Todo: Clear to black.
}

LRESULT CALLBACK main_window_callback(HWND window_handle, UINT message, WPARAM w_param, LPARAM l_param) {
    LRESULT result = 0;
    switch (message) {
       case WM_SIZE: {
           break;
       }
       case WM_QUIT: {
           should_quit = true;
           break;
       }
       case WM_DESTROY: { // When windows destroys our window
           should_quit = true;
           break;
       }
       case WM_CLOSE: { // When the user clicks the x.
           should_quit = true;
           break;
       }
       case WM_ACTIVATEAPP: { // If we are the active application
           output_debug_string("activate\n");
           break;
       }
       case WM_PAINT: {
           PAINTSTRUCT paint;
           HDC device_context = begin_paint(window_handle, &paint);

           Window_Dimension window_dimension = get_window_dimension(window_handle);

           swap_buffers(&back_buffer, device_context, window_dimension.width, window_dimension.height);
           end_paint(window_handle, &paint);
           break;
       }
       default: {
           result = default_window_procedure(window_handle, message, w_param, l_param);
       }
    }
    return result;
}

bool update_keyboard_events(Keyboard *keyboard, MSG *window_message) {
    // Previous key state.
    bool key_was_down = ((window_message->lParam  & (1 << 30)) != 0);
    // transition state.
    bool key_is_down  = ((window_message->lParam  & (1 << 31)) == 0);
    // context code
    b32 alt_key_is_down = (window_message->lParam & (1 << 29));

    u32 VK_CODE = window_message->wParam;
    bool handled = false;

    switch (window_message->message) {
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            keyboard->keyboard_buttons[VK_CODE].is_down = key_is_down;
            ++keyboard->keyboard_buttons[VK_CODE].half_transition_count;

            if (VK_CODE == VK_ESCAPE) {
                should_quit = true;
            } else if (VK_CODE == VK_F4 && alt_key_is_down) {
                should_quit = true;
            } else if (VK_CODE == VK_F5) {
                should_quit = true;
            } else if (VK_CODE == VK_F6) {

                // @Test Code:
                char *wav_file_full_path = "./data/test3/music_test.wav";
                char *wav_file_name      = "music_test.wav";
                play_sound(&sound_manager, wav_file_name);
            } else if (VK_CODE == VK_F11) {
                toggle_full_screen(window_message->hwnd);
            }
            handled = true;
            break;
        }
        case WM_SYSKEYUP:
        case WM_KEYUP: {
            keyboard->keyboard_buttons[VK_CODE].is_down = key_is_down;
            ++keyboard->keyboard_buttons[VK_CODE].half_transition_count;
            handled = true;
            break;
        }
        default: {
            break;
        }
    }

    if (handled) {
        if (keyboard->keyboard_buttons[VK_DOWN].is_down) {
            Y_OFFSET += 10;
        } else if (keyboard->keyboard_buttons[VK_UP].is_down) {
            Y_OFFSET -= 10;
        } else if (keyboard->keyboard_buttons[VK_LEFT].is_down) {
            X_OFFSET -= 10;
        } else if (keyboard->keyboard_buttons[VK_RIGHT].is_down) {
            X_OFFSET += 10;
        }
    }

    return handled;
}

bool update_mouse_events(Mouse *mouse, MSG *window_message) {
    bool handled = false;

    switch (window_message->message) {
        POINT point;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN: {
             GetCursorPos(&point);
             printf("%d %d\n", (s32)point.x, (s32)point.y);

            if (window_message->wParam & MK_LBUTTON) {
                output_debug_string("left down\n");
                mouse->left.is_down = true;
            }
            if (window_message->wParam & MK_RBUTTON) {
                output_debug_string("right down\n");
                mouse->right.is_down = true;
            }

            handled = true;
            break;
        }
        case WM_LBUTTONUP:
        case WM_RBUTTONUP: {
             GetCursorPos(&point);
             printf("%d %d\n", (s32)point.x, (s32)point.y);
            if ((window_message->wParam & MK_LBUTTON)) {
                output_debug_string("left up\n");
                mouse->left.is_down = false;
            }
            if ((window_message->wParam & MK_RBUTTON)) {
                output_debug_string("right up\n");
                mouse->right.is_down = false;
            }
            handled = true;
            break;
        }
        default: {
            break;
        }
    }
    return handled;
}

void update_window_events(Keyboard *keyboard, Mouse *mouse) {
    MSG window_message;

    bool is_keyboard_event = false;
    bool is_mouse_event    = false;
    while (peek_message(&window_message, null, 0, 0, PM_REMOVE)) {
        is_keyboard_event = update_keyboard_events(keyboard, &window_message);
        is_mouse_event    = update_mouse_events(mouse, &window_message);
        if (!is_keyboard_event && !is_mouse_event) {
            translate_and_dispatch_message(&window_message);
        }
    }
}

void read_game_controller_input() {
    // Read game controller buttons.
    for (s32 controller_index = 0; controller_index < XUSER_MAX_COUNT; ++controller_index) {
        XINPUT_STATE controller_state = {};
        auto result = XInputGetState(controller_index, &controller_state);
        if (result == ERROR_SUCCESS) {
            XINPUT_GAMEPAD *game_pad = &controller_state.Gamepad;

            bool dpad_up        = game_pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
            bool dpad_down      = game_pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
            bool dpad_left      = game_pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
            bool dpad_right     = game_pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
            bool start          = game_pad->wButtons & XINPUT_GAMEPAD_START;
            bool back           = game_pad->wButtons & XINPUT_GAMEPAD_BACK;
            bool left_shoulder  = game_pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
            bool right_shoulder = game_pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
            bool a_button       = game_pad->wButtons & XINPUT_GAMEPAD_A;
            bool b_button       = game_pad->wButtons & XINPUT_GAMEPAD_B;
            bool x_button       = game_pad->wButtons & XINPUT_GAMEPAD_X;
            bool y_button       = game_pad->wButtons & XINPUT_GAMEPAD_Y;

            s16 left_thumb_stick_x_axis = game_pad->sThumbLX;
            s16 left_thumb_stick_y_axis = game_pad->sThumbLY;

        } else {
            // Game controller isn't connected.
            //printf("%s\n", "Game controller isn't connected");
        }
    }
}

inline LARGE_INTEGER get_wall_clock() {
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline f32 get_seconds_elapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
    f32 result = ((f32)(end.QuadPart - start.QuadPart) / (f32)perf_count_freq);
    return result;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous_instance, PSTR command_line_arguments, INT command_show) {
    // Call before you create a window handle.
    PreventWindowsDPIScaling();

    u8 schedular_granularity_in_ms = 1;
    b32 sleep_is_granular = (timeBeginPeriod(schedular_granularity_in_ms) == TIMERR_NOERROR);

    LARGE_INTEGER perf_count_freq_result;
    QueryPerformanceFrequency(&perf_count_freq_result);
    perf_count_freq = perf_count_freq_result.QuadPart;

    load_xinput();

    // Create a window class
    WNDCLASSA window_class = create_window_class(CS_HREDRAW|CS_VREDRAW|CS_OWNDC, main_window_callback, null, null, instance, null, null, null, null, "WINDOW_NAME_CLASS");

    window_class.hCursor = LoadCursor(null, IDC_CROSS);

    create_backbuffer(&back_buffer, WINDOW_WIDTH, WINDOW_HEIGHT);

    // Register the windows class
    ATOM registered_window = register_window_class(&window_class);
    if (!registered_window) { printf("%s\n", "Failed to register window"); return 1; }

    // Create a window and get back a window handle
    HWND window_handle = create_window(null, window_class.lpszClassName, "water", WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, null, null, instance, null);

    if (!window_handle) { printf("%s\n", "Failed create a window"); return 1; }

    HDC device_context = get_dc(window_handle);

    // Should be 144 hertz.
    s32 monitor_refresh_rate = GetDeviceCaps(device_context, VREFRESH); 
    if (monitor_refresh_rate < 1) { 
        monitor_refresh_rate = 60;
    }

    //f32 game_update_hertz = monitor_refresh_rate / 2;
    f32 game_update_hertz = 30;
    f32 target_seconds_per_frame = 1.0f / game_update_hertz;

    char *file_name = "./data/test/test_background.bmp";
    Bitmap bitmap = {};
    bool success = load_bitmap_file(file_name, &bitmap);
    if (!success) { return 1; }

    char *hero_head_file_name = "./data/test/test_hero_front_head.bmp";
    Bitmap hero_bitmap = {};
    success = load_bitmap_file(hero_head_file_name, &hero_bitmap);
    if (!success) { return 1; }

    Keyboard keyboard = {};
    Mouse mouse       = {};

    LARGE_INTEGER last_counter = get_wall_clock();
    LARGE_INTEGER flip_wall_clock = {};

    char *wav_file_full_path_bloop = "./data/test3/music_test.wav";
    char *wav_file_name_bloop      = "music_test.wav";

    char *wav_file_full_path = "./data/test3/music_test.wav";
    char *wav_file_name      = "music_test.wav";


    init_sound_manager(&sound_manager, window_handle);

    // @Todo: actually compute this variance and
    // see what a reasonable value is.
    sound_manager.safety_bytes = (sound_manager.samples_per_second * sound_manager.bytes_per_sample / game_update_hertz);

    Sound_Data *sound_data = load_wav_file(wav_file_full_path, wav_file_name);

    if (!sound_data) { printf("Failed to load sound %s\n", wav_file_name); return 1; }

    array_add(&sound_manager.loaded_sounds, sound_data);

    s64 last_cycle_count = __rdtsc();

    play_sound(&sound_manager, wav_file_name);

    while (1) {
        if (should_quit) { break; }
        //TCHAR text[] = "Hello there";
        //TextOut(device_context, 50, 30, text, ARRAYSIZE(text));

        update_window_events(&keyboard, &mouse);

        read_game_controller_input();

        Game_Back_Buffer game_back_buffer = {};
        game_back_buffer.memory = back_buffer.memory;
        game_back_buffer.width  = back_buffer.width;
        game_back_buffer.height = back_buffer.height;
        game_back_buffer.pitch  = back_buffer.pitch;
        game_back_buffer.bpp    = back_buffer.bpp;

        Game_Memory game_memory = {};
        game_memory.persistent_memory_size = sizeof(Game_State);
        game_memory.persistent_memory      = VirtualAlloc(null, game_memory.persistent_memory_size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

        game_update_and_render(&game_memory, &game_back_buffer);

        game_draw_bitmap(&game_back_buffer, &bitmap,      0,        0);
        game_draw_bitmap(&game_back_buffer, &hero_bitmap, X_OFFSET, Y_OFFSET);
        
        LARGE_INTEGER audio_wall_clock = get_wall_clock();
        f32 from_beginning_of_frame_to_audio_seconds = get_seconds_elapsed(flip_wall_clock, audio_wall_clock);
        
        output_sound(&sound_manager);

        LARGE_INTEGER work_counter = get_wall_clock();
        f32 work_seconds_elapsed   = get_seconds_elapsed(last_counter, work_counter);

        f32 seconds_elapsed_for_frame = work_seconds_elapsed;
        if (seconds_elapsed_for_frame < target_seconds_per_frame) {
            if (sleep_is_granular) {
                DWORD sleep_ms = (DWORD)(1000.0f * (target_seconds_per_frame - seconds_elapsed_for_frame));
                if (sleep_ms > 0) { Sleep(sleep_ms); }
            }
            while (seconds_elapsed_for_frame < target_seconds_per_frame) {
                // How many ms left to wait.
                seconds_elapsed_for_frame = get_seconds_elapsed(last_counter, get_wall_clock());
            }
        } else {
            output_debug_string("MISSED FRAME RATE\n");
            // This means we missed our frame rate.
        }

        LARGE_INTEGER end_counter = get_wall_clock();
        f64 ms_per_frame = 1000.0f*get_seconds_elapsed(last_counter, end_counter);
        last_counter = end_counter;

        Window_Dimension window_dimension = get_window_dimension(window_handle);
        swap_buffers(&back_buffer, device_context, window_dimension.width, window_dimension.height);
        
        flip_wall_clock = get_wall_clock();

        s64 end_cycle_count = __rdtsc();
        u64 cycles_elapsed  = end_cycle_count - last_cycle_count;
        last_cycle_count    = end_cycle_count;

        f64 fps = 0.0f;
        f64 mega_cycles_per_frame = (f64)(cycles_elapsed  / (1000.0f * 1000.0f));

        //char buffer[256];
        //sprintf(buffer, "%.02fms/f,   %.02ff/s,   %.02fmc/f\n", ms_per_frame, fps, mega_cycles_per_frame);
        //output_debug_string(buffer);
    }

    // Sound_Manager cleanup:
    deinit_sound_manager(&sound_manager);

    return 0;
}
