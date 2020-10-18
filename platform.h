
/* These should be defined for each platform.
   Their values should have the same names on all. */
//enum virtual_key;
//enum window_show_mode;

#define PLATFORM_MB_LEFT 1
#define PLATFORM_MB_RIGHT 2
#define PLATFORM_MB_MIDDLE 3


struct Window;
struct Mutex;
struct Thread;
struct String;

enum virtual_key
{
    VKEY_NONE = 0,
    VKEY_BACKSPACE,
    VKEY_TAB,
    VKEY_RETURN,
    VKEY_SHIFT,
    VKEY_CONTROL,
    VKEY_ALT,
    VKEY_CAPSLOCK,
    VKEY_ESCAPE,
    VKEY_SPACE,
    VKEY_PAGE_UP,
    VKEY_PAGE_DOWN,
    VKEY_END,
    VKEY_HOME,
    VKEY_LEFT,
    VKEY_UP,
    VKEY_RIGHT,
    VKEY_DOWN,
    VKEY_INSERT,
    VKEY_DELETE,
    VKEY_ZERO,
    VKEY_ONE,
    VKEY_TWO,
    VKEY_THREE,
    VKEY_FOUR,
    VKEY_FIVE,
    VKEY_SIX,
    VKEY_SEVEN,
    VKEY_EIGHT,
    VKEY_NINE,
    VKEY_F1,
    VKEY_F2,
    VKEY_F3,
    VKEY_F4,
    VKEY_F5,
    VKEY_F6,
    VKEY_F7,
    VKEY_F8,
    VKEY_F9,
    VKEY_F10,
    VKEY_F11,
    VKEY_F12,
    VKEY_ADD,
    VKEY_MULTIPLY,
    VKEY_SUBTRACT,
    VKEY_DIVIDE,

    VKEY_a,
    VKEY_b,
    VKEY_c,
    VKEY_d,
    VKEY_e,
    VKEY_f,
    VKEY_g,
    VKEY_h,
    VKEY_i,
    VKEY_j,
    VKEY_k,
    VKEY_l,
    VKEY_m,
    VKEY_n,
    VKEY_o,
    VKEY_p,
    VKEY_q,
    VKEY_r,
    VKEY_s,
    VKEY_t,
    VKEY_u,
    VKEY_v,
    VKEY_w,
    VKEY_x,
    VKEY_y,
    VKEY_z,

    PLATFORM_N_VIRTUAL_KEYS
};

enum Mouse_Button
{
    MB_PRIMARY   = 0b00001, // Usually left
    MB_SECONDARY = 0b00010, // Usually right
    MB_AUXILARY  = 0b00100, // Usually middle or mouse wheel button
    MB_FOURTH    = 0b01000, // Typically the "Browser Back" button
    MB_FIFTH     = 0b10000  // Typically the "Browser Forward" button
};


struct Window_Delegate
{
    void *data;
    
    void (*key_down)(Window *window, virtual_key Key, u8 ScanCode, u16 repeat_count, void *Data);
    void (*key_up)  (Window *window, virtual_key Key, u8 ScanCode, void *Data);
    
    void (*character_input)(Window *window, byte *UTF8, int NBytes, u16 RepeatCount, void *Data);
    
    void (*mouse_down)(Window *window, Mouse_Button Button, void *Data);
    void (*mouse_up)(Window *window, Mouse_Button Button, void *Data);
    void (*mouse_move)(Window *window, int X, int Y, u64 ms, void *Data);
};



#if OS_WINDOWS
s64 platform_performance_counter();
s64 platform_performance_counter_frequency();
#endif
