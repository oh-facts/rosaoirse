#define YK_PLATFORM_DESKTOP
#include <platform/yk_platform.h>

#include <Windows.h>
#include <windowsx.h>


#define WIN_SIZE_X (800)
#define WIN_SIZE_Y (600)

struct win_state
{
    u32 is_running;
    struct YkInput input;
};

internal void win32_set_title(void *win, const char *title)
{
    SetWindowText(win,title);
}

internal LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int main(int argc, char *argv[])
{
    SetProcessDPIAware();
    WNDCLASS wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = GetModuleHandle(0);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "MainWindowClass";
    
    AssertM(RegisterClass(&wc), "win32 register class failed");
    
    struct win_state win_state = {0};
    
    HWND win = CreateWindowA(wc.lpszClassName, "michaelsoft bindows", WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT, CW_USEDEFAULT, WIN_SIZE_X, WIN_SIZE_Y,
                             NULL, NULL, wc.hInstance, &win_state);
    
    AssertM(win, "Failed to create window");
    
    /*
        0 is keyboard
        1 is mouse
    */
    RAWINPUTDEVICE devices[2] = {0};
    
    devices[0].usUsagePage = 0x01;
    devices[0].usUsage = 0x06;
    devices[0].dwFlags = 0;
    devices[0].hwndTarget = win;
    
    devices[1].usUsagePage = 0x01;
    devices[1].usUsage = 0x02;
    devices[1].dwFlags = 0;
    devices[1].hwndTarget = win;
    
    RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
    
    ShowWindow(win, SW_SHOWNORMAL);
    UpdateWindow(win);
    win_state.is_running = 1;
    
    // platform
    struct YkPlatform platform = {0};
    
    size_t mem_size = Megabytes(100);
    platform.memory = malloc(mem_size);
    platform.mem_size = mem_size;
    platform.scratch = malloc(mem_size);
    platform.scratch_size = mem_size;
    
    platform._win = win;
    platform.innit_audio = miniaudio_innit_audio;
    platform.play_audio = miniaudio_play_audio;
    platform.stop_audio = miniaudio_stop_audio;
    platform.set_title = win32_set_title;
    platform.read_file = yk_read_binary_file;
    
    struct offscreen_buffer render_target = {0};
    render_target.height = 600;
    render_target.width = 800;
    render_target.pixels = malloc(sizeof(u32) * 800 * 600);
    
    HMODULE dll = LoadLibraryA("yk3.dll");
    AssertM(dll,"failed to load dll yk3.dll");
    
    yk_innit_game_func innit_game = (yk_innit_game_func)GetProcAddress(dll, "yk_innit_game");
    yk_update_and_render_game_func update_and_render_game = (yk_update_and_render_game_func)GetProcAddress(dll, "yk_update_and_render_game");
    
    innit_game(&platform, &render_target);
    LARGE_INTEGER start_counter = {0};
    QueryPerformanceCounter(&start_counter);
    
    LARGE_INTEGER perf_freq = {0};
    QueryPerformanceFrequency(&perf_freq);
    i64 counter_freq = perf_freq.QuadPart;
    
    f64 total_time_elapsed = 0;
    f64 dt = 0;
    
    while (win_state.is_running)
    {
        f64 last_time_elapsed = total_time_elapsed;
        
        MSG message = {0};
        while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
        
        
        update_and_render_game(&platform, &render_target, &win_state.input,  dt);
        
        
        // basic window blit
        HDC hdc = GetDC(win);
        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = 800;
        bmi.bmiHeader.biHeight = -600;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32; 
        bmi.bmiHeader.biCompression = BI_RGB;
        
        StretchDIBits(hdc, 0, 0, 800, 600, 0, 0, 800, 600, render_target.pixels, &bmi, DIB_RGB_COLORS, SRCCOPY);
        
        ReleaseDC(win, hdc);
        
        for (u32 i = 0; i < YK_ACTION_COUNT; i++)
        {
            win_state.input.keys_old[i] = win_state.input.keys[i];
        }
        
        LARGE_INTEGER end_counter = {0};
        QueryPerformanceCounter(&end_counter);
        
        i64 counter_elapsed = end_counter.QuadPart - start_counter.QuadPart;
        total_time_elapsed = (1.f * counter_elapsed) / counter_freq;
        
        dt = total_time_elapsed - last_time_elapsed;
        
        // sleeping to limit cycles
        local_persist f32 counter = 0;
        counter += dt;
        
        if (counter <= 1.0 / 60)
        {
            Sleep((DWORD)((1.0 / 60 - counter) * 1000));
        }
        else
        {
            counter = 0;
        }
        
    }
    
    return 0;
}


internal LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    
    switch (msg)
    {
        
        // https://blog.molecular-matters.com/2011/09/05/properly-handling-keyboard-input/
        case WM_INPUT:
        {
            struct win_state *state = (struct win_state *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            
            char buffer[sizeof(RAWINPUT)] = {0};
            UINT size = sizeof(RAWINPUT);
            
            GetRawInputData((HRAWINPUT)(lParam), RID_INPUT, buffer, &size, sizeof(RAWINPUTHEADER));
            
            RAWINPUT *raw = (RAWINPUT *)(buffer);
            if (raw->header.dwType == RIM_TYPEKEYBOARD)
            {
                
                const RAWKEYBOARD *rawKB = &raw->data.keyboard;
                
                UINT virtualKey = rawKB->VKey;
                UINT scanCode = rawKB->MakeCode;
                UINT flags = rawKB->Flags;
                
                const i32 wasUp = ((flags & RI_KEY_BREAK) != 0);
                
                if (virtualKey == 255)
                {
                    // discard "fake keys" which are part of an escaped sequence
                    return 0;
                }
                else if (virtualKey == VK_SHIFT)
                {
                    // correct left-hand / right-hand SHIFT
                    virtualKey = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);
                }
                else if (virtualKey == VK_NUMLOCK)
                {
                    // correct PAUSE/BREAK and NUM LOCK silliness, and set the extended bit
                    scanCode = (MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC) | 0x100);
                }
                
                const i32 isE0 = ((flags & RI_KEY_E0) != 0);
                const i32 isE1 = ((flags & RI_KEY_E1) != 0);
                
                if (isE1)
                {
                    // for escaped sequences, turn the virtual key into the correct scan code using MapVirtualKey.
                    // however, MapVirtualKey is unable to map VK_PAUSE (this is a known bug), hence we map that by hand.
                    if (virtualKey == VK_PAUSE)
                        scanCode = 0x45;
                    else
                        scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
                }
                
                switch (virtualKey)
                {
                    // right-hand CONTROL and ALT have their e0 bit set
                    case VK_CONTROL:
                    if (isE0)
                        virtualKey = VK_RCONTROL;
                    else
                        virtualKey = VK_LCONTROL;
                    break;
                    case VK_MENU:
                    if (isE0)
                        virtualKey = VK_RMENU;
                    else
                        virtualKey = VK_LMENU;
                    break;
                    
                    // NUMPAD ENTER has its e0 bit set
                    /*
                    * I don't care for numpad enter
                case VK_RETURN:
                    if (isE0)
                        virtualKey = VK_NUMPAD_ENTER (FUCK MICROSOFT!);
                    break;
                    */
                    // the standard INSERT, DELETE, HOME, END, PRIOR and NEXT keys will always have their e0 bit set, but the
                    // corresponding keys on the NUMPAD will not.
                    case VK_INSERT:
                    if (!isE0)
                        virtualKey = VK_NUMPAD0;
                    break;
                    /*
                    case VK_DELETE:
                        if (!isE0)
                            virtualKey = VK_NUMPADP_DECIMAL// (FUCK MICROSOFT!);
                        break;
                    */
                    case VK_HOME:
                    if (!isE0)
                        virtualKey = VK_NUMPAD7;
                    break;
                    
                    case VK_END:
                    if (!isE0)
                        virtualKey = VK_NUMPAD1;
                    break;
                    
                    case VK_PRIOR:
                    if (!isE0)
                        virtualKey = VK_NUMPAD9;
                    break;
                    
                    case VK_NEXT:
                    if (!isE0)
                        virtualKey = VK_NUMPAD3;
                    break;
                    
                    // the standard arrow keys will always have their e0 bit set, but the
                    // corresponding keys on the NUMPAD will not.
                    case VK_LEFT:
                    if (!isE0)
                        virtualKey = VK_NUMPAD4;
                    break;
                    
                    case VK_RIGHT:
                    if (!isE0)
                        virtualKey = VK_NUMPAD6;
                    break;
                    
                    case VK_UP:
                    {
                        if (!isE0)
                        {
                            virtualKey = VK_NUMPAD8;
                        }
                        else
                        {
                            // printf("%d\n",!wasUp);
                            state->input.keys[YK_ACTION_UP] = !wasUp;
                        }
                    }
                    break;
                    
                    case VK_DOWN:
                    if (!isE0)
                        virtualKey = VK_NUMPAD2;
                    break;
                    
                    // NUMPAD 5 doesn't have its e0 bit set
                    case VK_CLEAR:
                    if (!isE0)
                        virtualKey = VK_NUMPAD5;
                    break;
                }
                
                //  ->keys._cur[virtualKey] = !wasUp;
                
                // getting a human-readable string
#if 0
                UINT key = (scanCode << 16) | (isE0 << 24);
                char buffer[512] = {};
                GetKeyNameText((LONG)key, buffer, 512);
                
                printf("%d %s %d\n", virtualKey, buffer, wasUp);
#endif
            }
            /*
            else if (raw->header.dwType == RIM_TYPEMOUSE)
            {
                if (raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_DOWN)
                {
                    win->clicks._cur[YK_MOUSE_BUTTON_LEFT] = 1;
                }
                else if (raw->data.mouse.usButtonFlags == RI_MOUSE_LEFT_BUTTON_UP)
                {
                    win->clicks._cur[YK_MOUSE_BUTTON_LEFT] = 0;
                }
    
                if (raw->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_DOWN)
                {
                    win->clicks._cur[YK_MOUSE_BUTTON_RIGHT] = 1;
                }
                else if (raw->data.mouse.usButtonFlags == RI_MOUSE_RIGHT_BUTTON_UP)
                {
                    win->clicks._cur[YK_MOUSE_BUTTON_RIGHT] = 0;
                }
    
                win->mouse_pos.rel = v2{(f32)raw->data.mouse.lLastX, (f32)raw->data.mouse.lLastY};
    
                // printf("%ld\n", raw->data.mouse.lLastX);
            }*/
        }
        break;
        case WM_MOUSEMOVE:
        {
            i32 x_pos = GET_X_LPARAM(lParam);
            i32 y_pos = GET_Y_LPARAM(lParam);
            /*
            POINT point = { (LONG)(win->win_data.size_x / 2.f),(LONG)(win->win_data.size_y / 2.f) };
    
            ClientToScreen((HWND)win->win, &point);
            SetCursorPos(point.x, point.y);
            */
            // ykm_print_v2(yk_input_mouse_mv(&win->mouse_pos));
        }
        break;
        /*
        * Currently discarded these events from shooting entirely since I don't need them. Will be relevant when I start with UI
        case WM_KEYDOWN:
        {
            YkWindow* win = (YkWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            switch (wParam)
            {
                case VK_ESCAPE:
                {
                    win->test = 1;
                }break;
    
                case 0x41:
                {
                    printf("A pressed");
                }
    
            }
        }break;
        */
        case WM_CREATE:
        {
            struct win_state *state = (struct win_state *)((CREATESTRUCT *)lParam)->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)state);
        }
        break;
        
        case WM_CLOSE:
        {
            struct win_state *state = (struct win_state *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            state->is_running = 0;
            PostQuitMessage(0);
        }
        break;
        
        case WM_SIZE:
        {
            if (wParam == SIZE_MINIMIZED)
            {
            }
            else if (wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)
            {
            }
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
