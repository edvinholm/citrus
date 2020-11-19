
enum Tweak_Type {
    TWEAK_TYPE_BOOL,
    TWEAK_TYPE_FLOAT,
    TWEAK_TYPE_INT,
    TWEAK_TYPE_UINT,
};

enum Tweak_ID {
    // GRAPHICS //
    TWEAK_VSYNC,
    TWEAK_MAX_MULTISAMPLE_SAMPLES,
    TWEAK_FONT_TEXTURE_SIZE,
    TWEAK_FONT_OVERSAMPLING_RATE,

    // APPEARANCE //
    TWEAK_WINDOW_BORDER_COLOR, // @Temporary?
    
    // //
    TWEAK_INITIAL_OS_WINDOW_RECT,

    // //
    TWEAK_SCROLL_TO_CARET_REPEAT_INTERVAL,

    TWEAK_NONE_OR_NUM
};

struct Tweak_Info {
    const char *name;
    Tweak_Type type;
    u8 num_components;
};

const u8 MAX_TWEAK_VALUE_COMPONENTS = 8;

struct Tweak_Value {
    union {
        bool  bool_values [MAX_TWEAK_VALUE_COMPONENTS];
        float float_values[MAX_TWEAK_VALUE_COMPONENTS];
        s32   int_values  [MAX_TWEAK_VALUE_COMPONENTS];
        u32   uint_values [MAX_TWEAK_VALUE_COMPONENTS];
    };
};

struct Tweaks
{
    Tweak_Info infos[TWEAK_NONE_OR_NUM] = {
        // GRAPHICS //
        { "vsync", TWEAK_TYPE_BOOL, 1 },
        { "max_multisample_samples", TWEAK_TYPE_INT,   1 },
        { "font_texture_size",       TWEAK_TYPE_UINT,  1 },
        { "font_oversampling_rate",  TWEAK_TYPE_FLOAT, 1 },

        // APPEARANCE //
        { "window_border_color", TWEAK_TYPE_FLOAT, 4 },

        // //
        { "initial_os_window_rect", TWEAK_TYPE_INT, 4 },

        // //
        { "scroll_to_caret_repeat_interval", TWEAK_TYPE_FLOAT, 1 }
    };

    Tweak_Value values[ARRLEN(infos)] = {0};

#if OS_WINDOWS
    HANDLE dir_change_notif;
#endif
};

Tweaks tweaks;
