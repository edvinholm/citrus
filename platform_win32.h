
struct Mutex
{
    HANDLE handle;
};

struct Thread
{
    HANDLE handle;
};

struct Socket
{
    SOCKET handle;
};
bool equal(Socket *a, Socket *b)
{
    return a->handle == b->handle;
}


u32 platform_big_endian_32(u32 int_with_machine_endianness);
u16 platform_big_endian_16(u16 int_with_machine_endianness);
 
u32 platform_machine_endian_from_big_32(u32 big_endian_int);
u16 platform_machine_endian_from_big_16(u16 big_endian_int);



void platform_create_mutex(Mutex *_mutex);
void platform_lock_mutex(Mutex *mutex);
void platform_unlock_mutex(Mutex *mutex);
void platform_delete_mutex(Mutex *mutex);

u64 platform_milliseconds();



//TODO These can probably change between keyboard layouts???
#define KEY_SPACE  32

#define KEY_ARROW_LEFT  37
#define KEY_ARROW_UP    38
#define KEY_ARROW_RIGHT 39
#define KEY_ARROW_DOWN  40

// NOTE: Get more from:
// https://docs.microsoft.com/en-us/windows/desktop/inputdev/virtual-key-codes
/*enum virtual_key
{
    VKEY_BACKSPACE = VK_BACK,
    VKEY_TAB = VK_TAB,
    VKEY_RETURN = VK_RETURN,
    VKEY_SHIFT = VK_SHIFT,
    VKEY_CONTROL = VK_CONTROL,
    VKEY_ALT = VK_MENU,
    VKEY_CAPSLOCK = VK_CAPITAL,
    VKEY_ESCAPE = VK_ESCAPE,
    VKEY_SPACE = VK_SPACE,
    VKEY_PAGE_UP = VK_PRIOR,
    VKEY_PAGE_DOWN = VK_NEXT,
    VKEY_END = VK_END,
    VKEY_HOME = VK_HOME,
    VKEY_LEFT = VK_LEFT,
    VKEY_UP = VK_UP,
    VKEY_RIGHT = VK_RIGHT,
    VKEY_DOWN = VK_DOWN,
    VKEY_INSERT = VK_INSERT,
    VKEY_DELETE = VK_DELETE,
    VKEY_ZERO  = 0x30,
    VKEY_ONE   = 0x31,
    VKEY_TWO   = 0x32,
    VKEY_THREE = 0x33,
    VKEY_FOUR  = 0x34,
    VKEY_FIVE  = 0x35,
    VKEY_SIX   = 0x36,
    VKEY_SEVEN = 0x37,
    VKEY_EIGHT = 0x38,
    VKEY_NINE  = 0x39,
    VKEY_F1  = VK_F1,
    VKEY_F2  = VK_F2,
    VKEY_F3  = VK_F3,
    VKEY_F4  = VK_F4,
    VKEY_F5  = VK_F5,
    VKEY_F6  = VK_F6,
    VKEY_F7  = VK_F7,
    VKEY_F8  = VK_F8,
    VKEY_F9  = VK_F9,
    VKEY_F10 = VK_F10,
    VKEY_F11 = VK_F11,
    VKEY_F12 = VK_F12,
    VKEY_ADD = VK_ADD,
    VKEY_MULTIPLY = VK_MULTIPLY,
    VKEY_SUBTRACT = VK_SUBTRACT,
    VKEY_DIVIDE = VK_DIVIDE
};
*/

enum window_show_mode
{
    WSHOW_MAXIMIZE = SW_MAXIMIZE,
    WSHOW_MINIMIZE = SW_MINIMIZE,
    WSHOW_RESTORE = SW_RESTORE
};



