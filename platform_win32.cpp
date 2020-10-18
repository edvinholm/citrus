



struct Window
{
    HWND Handle;
    HGLRC GLContext;
    HDC DeviceContext;
    
    //@Hack @Ugly
    bool CloseButtonClicked;
    
    Window_Delegate delegate;
};

struct Thread
{
    HANDLE handle;
};




//@Hack
Window *WIN32_DEFAULT_INPUT_WINDOW = 0;



inline
virtual_key _win32_windows_vkey_to_virtual_key(u32 Key)
{
    switch(Key)
    {
        case VK_BACK: return VKEY_BACKSPACE; 
        case VK_TAB: return VKEY_TAB; 
        case VK_RETURN: return VKEY_RETURN; 
        case VK_SHIFT: return VKEY_SHIFT; 
        case VK_CONTROL: return VKEY_CONTROL;
            
        case VK_LMENU: 
        case VK_RMENU: 
        case VK_MENU:
        {
            return VKEY_ALT;
        }
        
        case VK_CAPITAL: return VKEY_CAPSLOCK; 
        case VK_ESCAPE: return VKEY_ESCAPE; 
        case VK_SPACE: return VKEY_SPACE; 
        case VK_PRIOR: return VKEY_PAGE_UP; 
        case VK_NEXT: return VKEY_PAGE_DOWN; 
        case VK_END: return VKEY_END; 
        case VK_HOME: return VKEY_HOME; 
        case VK_LEFT: return VKEY_LEFT; 
        case VK_UP: return VKEY_UP; 
        case VK_RIGHT: return VKEY_RIGHT; 
        case VK_DOWN: return VKEY_DOWN; 
        case VK_INSERT: return VKEY_INSERT; 
        case VK_DELETE: return VKEY_DELETE; 
        case 0x30: return VKEY_ZERO; 
        case 0x31: return VKEY_ONE; 
        case 0x32: return VKEY_TWO; 
        case 0x33: return VKEY_THREE; 
        case 0x34: return VKEY_FOUR; 
        case 0x35: return VKEY_FIVE; 
        case 0x36: return VKEY_SIX; 
        case 0x37: return VKEY_SEVEN; 
        case 0x38: return VKEY_EIGHT; 
        case 0x39: return VKEY_NINE; 
        case VK_F1: return VKEY_F1; 
        case VK_F2: return VKEY_F2; 
        case VK_F3: return VKEY_F3; 
        case VK_F4: return VKEY_F4; 
        case VK_F5: return VKEY_F5; 
        case VK_F6: return VKEY_F6; 
        case VK_F7: return VKEY_F7; 
        case VK_F8: return VKEY_F8; 
        case VK_F9: return VKEY_F9; 
        case VK_F10: return VKEY_F10; 
        case VK_F11: return VKEY_F11; 
        case VK_F12: return VKEY_F12; 
        case VK_ADD: return VKEY_ADD; 
        case VK_MULTIPLY: return VKEY_MULTIPLY; 
        case VK_SUBTRACT: return VKEY_SUBTRACT; 
        case VK_DIVIDE: return VKEY_DIVIDE;

            
        case 0x41: return VKEY_a;
        case 0x42: return VKEY_b;
        case 0x43: return VKEY_c;
        case 0x44: return VKEY_d;
        case 0x45: return VKEY_e;
        case 0x46: return VKEY_f;
        case 0x47: return VKEY_g;
        case 0x48: return VKEY_h;
        case 0x49: return VKEY_i;
        case 0x4A: return VKEY_j;
        case 0x4B: return VKEY_k;
        case 0x4C: return VKEY_l;
        case 0x4D: return VKEY_m;
        case 0x4E: return VKEY_n;
        case 0x4F: return VKEY_o;
        case 0x50: return VKEY_p;
        case 0x51: return VKEY_q;
        case 0x52: return VKEY_r;
        case 0x53: return VKEY_s;
        case 0x54: return VKEY_t;
        case 0x55: return VKEY_u;
        case 0x56: return VKEY_v;
        case 0x57: return VKEY_w;
        case 0x58: return VKEY_x;
        case 0x59: return VKEY_y;
        case 0x5A: return VKEY_z;
            
    }
    
    return VKEY_NONE;
}


#define CHAR_BACKSPACE 8


//TODO @CPort: Do what nCmdShow says! @Robustness

//
// How do we get the window from HWND?
//
LRESULT CALLBACK _win32_main_window_callback(
HWND Window,
UINT Message,
WPARAM WParam,
LPARAM LParam
)
{
    
    LRESULT Result = 0;
    
    switch (Message)
    {
        case WM_SIZE:
        {
            return DefWindowProc(Window, Message, WParam, LParam);
        } break;


        case WM_CHAR:
        {
            WCHAR Codepoint = WParam;
            u16 RepeatCount = LParam & 0xFFFF;
            
            byte UTF8[8] = {0};
            int NUTF8Bytes = WideCharToMultiByte(CP_UTF8, 0, &Codepoint, 1,
                                                 (LPSTR)UTF8, 8, NULL, NULL);

            if(NUTF8Bytes == 0)
            {
#if DEBUG
                printf("WideCharToMultiByte failed. Error code: %lu\n", GetLastError());
#endif
                return 0;
            }

            auto Proc = WIN32_DEFAULT_INPUT_WINDOW->delegate.character_input;
            if(Proc)
            {
                Proc(WIN32_DEFAULT_INPUT_WINDOW,
                     UTF8, NUTF8Bytes, RepeatCount,
                     WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
            }

            return 0;
        }
        break;
        
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            if(!WIN32_DEFAULT_INPUT_WINDOW) return DefWindowProc(Window, Message, WParam, LParam);
            
            virtual_key VirtualKey = _win32_windows_vkey_to_virtual_key(WParam);
            u16 RepeatCount = LParam & 0xFFFF;
            u8 ScanCode = (LParam & 0xFF0000) >> 16;
            bool ExtendedKey = (LParam & 0x1000000) >> 24;
            
            if(Message == WM_KEYDOWN)
            {
                auto Proc = WIN32_DEFAULT_INPUT_WINDOW->delegate.key_down;
                if(Proc)
                {
                    Proc(WIN32_DEFAULT_INPUT_WINDOW,
                         VirtualKey, ScanCode, RepeatCount,
                         WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
                }
            }
            else
            {
                auto Proc = WIN32_DEFAULT_INPUT_WINDOW->delegate.key_up;
                if(Proc)
                {
                    Proc(WIN32_DEFAULT_INPUT_WINDOW,
                         VirtualKey, ScanCode,
                         WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
                }
            }
            
            
        } break;
        
        case WM_DESTROY:
        {
            return DefWindowProc(Window, Message, WParam, LParam);
        } break;
        
        case WM_CLOSE:
        {
            if(WIN32_DEFAULT_INPUT_WINDOW) 
            {
                WIN32_DEFAULT_INPUT_WINDOW->CloseButtonClicked = true;
                return 0;
            }
            return DefWindowProc(Window, Message, WParam, LParam);
        } break;
        
        case WM_PAINT:
        {
            return DefWindowProc(Window, Message, WParam, LParam);
        } break;
        
        case WM_ACTIVATEAPP:
        {
            return DefWindowProc(Window, Message, WParam, LParam);
        }break;
        
        case WM_MOUSEMOVE:
        {
            if(WIN32_DEFAULT_INPUT_WINDOW->delegate.mouse_move)
            {
                POINT Mouse;
                GetCursorPos(&Mouse);
                ScreenToClient(WIN32_DEFAULT_INPUT_WINDOW->Handle, &Mouse);
                WIN32_DEFAULT_INPUT_WINDOW->delegate.mouse_move(WIN32_DEFAULT_INPUT_WINDOW,
                                                                Mouse.x, Mouse.y, platform_milliseconds(),
                                                                WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
            }
            else
            {
                return DefWindowProc(Window, Message, WParam, LParam);
            }
            
        } break;
        
        case WM_LBUTTONDOWN:
            SetCapture(WIN32_DEFAULT_INPUT_WINDOW->Handle); 
            WIN32_DEFAULT_INPUT_WINDOW->delegate.mouse_down(WIN32_DEFAULT_INPUT_WINDOW, MB_PRIMARY, WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
        break;
        
        case WM_RBUTTONDOWN:
            SetCapture(WIN32_DEFAULT_INPUT_WINDOW->Handle); 
            WIN32_DEFAULT_INPUT_WINDOW->delegate.mouse_down(WIN32_DEFAULT_INPUT_WINDOW, MB_SECONDARY, WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
        break;
        
        case WM_MBUTTONDOWN:
            SetCapture(WIN32_DEFAULT_INPUT_WINDOW->Handle); 
            WIN32_DEFAULT_INPUT_WINDOW->delegate.mouse_down(WIN32_DEFAULT_INPUT_WINDOW, MB_AUXILARY, WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
        break;

        case WM_XBUTTONDOWN:
        {
            SetCapture(WIN32_DEFAULT_INPUT_WINDOW->Handle); 
            Mouse_Button Button= (GET_XBUTTON_WPARAM(WParam) == XBUTTON1) ? MB_FOURTH : MB_FIFTH;
            WIN32_DEFAULT_INPUT_WINDOW->delegate.mouse_down(WIN32_DEFAULT_INPUT_WINDOW, Button, WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
        } break;

        
        case WM_LBUTTONUP:
            ReleaseCapture(); 
            WIN32_DEFAULT_INPUT_WINDOW->delegate.mouse_up(WIN32_DEFAULT_INPUT_WINDOW, MB_PRIMARY, WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
        break;
        
        case WM_RBUTTONUP:
            ReleaseCapture(); 
            WIN32_DEFAULT_INPUT_WINDOW->delegate.mouse_up(WIN32_DEFAULT_INPUT_WINDOW, MB_SECONDARY, WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
        break;
        
        case WM_MBUTTONUP:
            ReleaseCapture(); 
            WIN32_DEFAULT_INPUT_WINDOW->delegate.mouse_up(WIN32_DEFAULT_INPUT_WINDOW, MB_AUXILARY, WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
        break;
        
        case WM_XBUTTONUP:
        {
            ReleaseCapture(); 
            Mouse_Button Button = (GET_XBUTTON_WPARAM(WParam) == XBUTTON1) ? MB_FOURTH : MB_FIFTH;
            WIN32_DEFAULT_INPUT_WINDOW->delegate.mouse_up(WIN32_DEFAULT_INPUT_WINDOW, Button, WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
        } break;
            
        
        case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO MinMaxInfo = (LPMINMAXINFO)LParam;
            MinMaxInfo->ptMinTrackSize.x = 100;
            MinMaxInfo->ptMinTrackSize.y = 100;
        }
        break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        {
            if(!WIN32_DEFAULT_INPUT_WINDOW) return DefWindowProc(Window, Message, WParam, LParam);
            
            virtual_key key = _win32_windows_vkey_to_virtual_key(WParam);
            
            u16 repeat_count = LParam & 0xFFFF;
            u8 scan_code = (LParam & 0xFF0000) >> 16;

            if(Message == WM_SYSKEYDOWN)
            {
                auto Proc = WIN32_DEFAULT_INPUT_WINDOW->delegate.key_down;
                if(Proc)
                {
                    Proc(WIN32_DEFAULT_INPUT_WINDOW,
                         key, scan_code, repeat_count,
                         WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
                }
            }
            else
            {
                auto Proc = WIN32_DEFAULT_INPUT_WINDOW->delegate.key_up;
                if(Proc)
                {
                    Proc(WIN32_DEFAULT_INPUT_WINDOW,
                         key, scan_code,
                         WIN32_DEFAULT_INPUT_WINDOW->delegate.data);
                }
            }
            
            return 0;
        } break;

        case WM_SYSCHAR:
        {
// NOTE: From: https://stackoverflow.com/questions/11623085/pressing-alt-hangs-the-application
            return TRUE;
        } break;
        
        default:
        {
            return DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

//NOTE: Returns true if the window should close.
bool platform_process_input(Window *window, u8 mouse_buttons_down /* @Hack */)
{
    MSG Message;
    while(PeekMessage(&Message, window->Handle, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
    }

    return window->CloseButtonClicked;
}

#if 0
inline
HGLRC _win32_init_opengl(HWND Window)
{
    HDC WindowDeviceContext = GetDC(Window);
    
    PIXELFORMATDESCRIPTOR DesiredPixelFormat =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
        PFD_TYPE_RGBA,             //The kind of framebuffer. RGBA or palette.
        32,                        //Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        16,//24,                       //Number of bits for the depthbuffer
        8,                        //Number of bits for the stencilbuffer
        0,                        //Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    
    int PixelFormatIndex = ChoosePixelFormat(
        WindowDeviceContext,
        &DesiredPixelFormat
                                             );
    PIXELFORMATDESCRIPTOR PixelFormat;
    DescribePixelFormat(WindowDeviceContext, PixelFormatIndex, sizeof(PixelFormat), &PixelFormat);
    
    SetPixelFormat(WindowDeviceContext, PixelFormatIndex, &PixelFormat);
    
    HGLRC OpenGLContext = wglCreateContext(WindowDeviceContext);
    if (wglMakeCurrent(WindowDeviceContext, OpenGLContext))
    {
        
    }
    else
    {
        //DEBUG
#if DEBUG
        MessageBox(Window, "wglMakeCurrent failed.", "Error", 0);
#endif
    }
    
    ReleaseDC(Window, WindowDeviceContext);
    
    return OpenGLContext;
}

#endif

//TODO: When destroying window:
/*
  ReleaseDC(WindowHandle, WindowDeviceContext);
*/


void platform_get_dpi(Window *window, u32 *_x, u32 *_y)
{
    HMONITOR monitor = MonitorFromWindow(window->Handle, MONITOR_DEFAULTTONEAREST);
    HRESULT fetch_result = GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, _x, _y);
    Assert(fetch_result == S_OK);
}



void platform_create_gl_window(Window *_window, const char *Title = "",
                               int Width = 640, int Height = 480, int X = CW_USEDEFAULT, int Y = CW_USEDEFAULT)
{
    Assert(false);
    
    
    HINSTANCE Instance = GetModuleHandle(0);
    
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_CLASSDC | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = _win32_main_window_callback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon = ;//LoadIcon(Instance, MAKEINTRESOURCE(IDI_WINLOGO));
    WindowClass.lpszClassName = "GLWindow";
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    
    if (RegisterClass(&WindowClass))
    {
        //Last parameter is passed with WM_CREATE message.
        HWND WindowHandle = CreateWindowEx(
            0,
            WindowClass.lpszClassName,
            Title,
            WS_EX_OVERLAPPEDWINDOW | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX, //WS_THICKFRAME makes the window resizable
            X,
            Y,
            Width,
            Height,
            0,
            0,
            Instance,
            0);
        
        if (WindowHandle)
        {
#if 0
            HGLRC OpenGLContext = _win32_init_opengl(WindowHandle);
            HDC WindowDeviceContext = GetDC(WindowHandle);
            
            HANDLE Icon = LoadImage(Instance, "res/icon.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
            SendMessage(WindowHandle, (UINT)WM_SETICON, ICON_BIG, (LPARAM)Icon);
            
            //TODO @CPort: Do what nCmdShow says!
            ShowWindow(WindowHandle, SW_SHOW);
            UpdateWindow(WindowHandle);
            
            Zero(*_window);
            
            _window->Handle = WindowHandle;
            _window->GLContext = OpenGLContext;
            _window->DeviceContext = WindowDeviceContext;
            
            if(!WIN32_DEFAULT_INPUT_WINDOW) WIN32_DEFAULT_INPUT_WINDOW = _window;
#endif
        }
    }
    
}

inline
void platform_get_window_size(Window *window, float *_Width, float *_Height)
{
    RECT Rect;
    GetClientRect(window->Handle, &Rect);
    *_Width = Rect.right - Rect.left;
    *_Height = Rect.bottom - Rect.top;
}


inline
void platform_get_window_position(Window *win, float *_x, float *_y)
{
    RECT rect;
    GetClientRect(win->Handle, &rect);
    *_x = rect.left;
    *_y = rect.top;
}

inline
void platform_get_window_rect(Window *win, float *_x, float *_y, float *_w, float *_h)
{
    RECT rect;
    GetClientRect(win->Handle, &rect);
    *_x = rect.left;
    *_y = rect.top;
    *_w = rect.right - rect.left;
    *_h = rect.bottom - rect.top;
}


inline
void platform_set_window_title(Window *window, String title)
{
    //@Robustness: Hmmmmm.... Doesn't SetWindowText expect a zero-terminated string?
    SetWindowText(window->Handle, (LPCSTR)title.data);
}

inline
bool platform_set_window_size(Window *window, int w, int h)
{
    return SetWindowPos(window->Handle, 0, 0, 0, w, h, SWP_NOMOVE | SWP_NOMOVE);   
}

inline
bool platform_set_window_position(Window *window, int x, int y)
{
    return SetWindowPos(window->Handle, 0, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);   
}

inline
bool platform_set_window_size_and_position(Window *window, int x, int y, int w, int h)
{
    return SetWindowPos(window->Handle, 0, x, y, w, h, SWP_NOZORDER);   
}


inline
bool platform_make_window_gpu_context_current(Window *window)
{
    // We do this when we create the window. When we tried to do it again, glGetError returned 1282.
    return false;
}

inline
void platform_begin_frame(Window *window)
{
    
}

inline
void platform_end_frame(Window *window)
{
    SwapBuffers(window->DeviceContext);
}


//IMPORTANT: This is not milliseconds since 1 Jan 1970!
inline
u64 platform_milliseconds()
{
    return GetTickCount64();
}


inline
s64 platform_performance_counter_frequency()
{
    LARGE_INTEGER i;
    QueryPerformanceFrequency(&i);
    return i.QuadPart;
}

inline
s64 platform_performance_counter()
{
    LARGE_INTEGER i;
    QueryPerformanceCounter(&i);
    return i.QuadPart;
}


bool platform_init()
{
    return true;
}


bool platform_create_thread(DWORD (*proc)(void *), void *param, Thread *_thread)
{
    Zero(*_thread);
    _thread->handle = CreateThread(NULL, 0, proc, param, 0, NULL);
    
    return (_thread->handle != NULL);
}

void platform_create_mutex(Mutex *_mutex)
{
    HANDLE mutex = CreateMutexA(NULL, false, NULL);
    _mutex->handle = mutex;
}


void platform_lock_mutex(Mutex *mutex)
{
    WaitForSingleObject(mutex->handle, INFINITE);
}

void platform_unlock_mutex(Mutex *mutex)
{
    ReleaseMutex(mutex->handle);
}

void platform_delete_mutex(Mutex *mutex)
{
    CloseHandle(mutex->handle);
}


bool platform_open_url(char *url)
{
    HINSTANCE instance = ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWMAXIMIZED);

    return instance > (HINSTANCE)32;
}

bool platform_sync_filesystem(bool load)
{
    return true;
}

String platform_get_resource_directory()
{
    return STRING("res/");
}

String platform_get_user_directory()
{
    return STRING("usr/");
}

float platform_get_status_bar_height()
{
    return 20;
}


u32 platform_big_endian_32(u32 int_with_machine_endianness) {
    return htonl(int_with_machine_endianness);
}
u16 platform_big_endian_16(u16 int_with_machine_endianness) {
    return htons(int_with_machine_endianness);
}

u32 platform_machine_endian_from_big_32(u32 big_endian_int) {
    return ntohl(big_endian_int);
}
u16 platform_machine_endian_from_big_16(u16 big_endian_int) {
    return ntohs(big_endian_int);
}


bool platform_can_encrypt()
{
    return false;
}

bool platform_encrypt(String to_encrypt, String *_encrypted, Allocator_ID allocator)
{
    Assert(platform_can_encrypt());
    //@Incomplete @NoReleaseWin32: Not implemented
    *_encrypted = copy_of(&to_encrypt, allocator);
    return true;
}

bool platform_decrypt(String to_decrypt, String *_decrypted, Allocator_ID allocator)
{
    Assert(platform_can_encrypt());
    //@Incomplete @NoReleaseWin32: Not implemented
    *_decrypted = copy_of(&to_decrypt, allocator);
    return true;
}


bool platform_get_iso_language(String *_iso, Allocator_ID allocator)
{
    u8 buf[3];
    int length = GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, (LPSTR)buf, sizeof(buf));
    if(length == 0) return false;

    _iso->data = (u8 *)alloc(sizeof(buf), allocator);
    memcpy(_iso->data, buf, length-1);
    _iso->length = length - 1; // It's zero-terminated

    return true;
}


