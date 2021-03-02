
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
    
    // //
    TWEAK_INITIAL_OS_WINDOW_RECT,

    // //
    TWEAK_SCROLL_TO_CARET_REPEAT_INTERVAL,

    // DEVELOPER //
    TWEAK_SHOW_PLAYER_PATHS,
    TWEAK_COLOR_TILES_BY_POSITION,
    TWEAK_STARTUP_USER,
    TWEAK_STARTUP_ROOM,
    TWEAK_SHOW_WINDOW_SIZES,
    TWEAK_SHOW_ENTITY_ACTION_POSITIONS,
    TWEAK_SHOW_PLAYER_ENTITY_PARTS,

    TWEAK_RUN_PROFILER,
    TWEAK_SHOW_PROFILER,
    TWEAK_PROFILER_YSCALE,
    //

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

        // //
        { "initial_os_window_rect", TWEAK_TYPE_INT, 4 },

        // //
        { "scroll_to_caret_repeat_interval", TWEAK_TYPE_FLOAT, 1 },

        // DEBUG //
        { "show_player_paths",       TWEAK_TYPE_BOOL, 1 },
        { "color_tiles_by_position", TWEAK_TYPE_BOOL, 1 },
        { "startup_user",            TWEAK_TYPE_UINT, 1 },
        { "startup_room",            TWEAK_TYPE_UINT, 1 },
        { "show_window_sizes",       TWEAK_TYPE_BOOL, 1 },
        { "show_entity_action_positions", TWEAK_TYPE_BOOL, 1 },
        { "show_player_entity_parts", TWEAK_TYPE_BOOL, 1 },
        
        { "run_profiler",            TWEAK_TYPE_BOOL,  1 },
        { "show_profiler",           TWEAK_TYPE_BOOL,  1 },
        { "profiler_yscale",         TWEAK_TYPE_FLOAT, 1 }
        //
    };
    static_assert(ARRLEN(infos) == TWEAK_NONE_OR_NUM);

    Tweak_Value values[ARRLEN(infos)] = {0};

#if OS_WINDOWS
    HANDLE dir_change_notif;
#endif
};

Tweaks tweaks;
