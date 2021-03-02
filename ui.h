
/*
  AUTOMATIC UI ID GENERATION
  --------------------------
  This is a messy solution, but it's the best way I've come
  up with for C++. And it's worth it to not need to worry
  about generating unique and reproducable IDs.

  It is this messy to help the programmer as much as possible and prevent them
  to do some of the most easily made mistakes. But it's nowhere near perfect,
  and you need to respect the rules of the systems in order for it to work.

  The same code path gives the same ID, basically.
  The paths are stored and mapped to ID integers.
  A path is a sequence of locations, identified by a "place" and a "count".
  The place component is an ID of a PACK site. The count is the number of
  packs in that place, in the current path...

  Example:     (The P() and PC() macros are the packing macros, and the U() macro is the unpack macro)
  void foo() {
      U(ctx);
      for(int i = 0; i < 10; i++)
      {
          button(PC(ctx, i)); // This is a "place" that will be hit multiple times for the same path, so we need to pass a "count" in to make 10 unique IDs here.
      }
  }
  void bar(UI_Context ctx) {
      U(ctx);
      foo(P(ctx));  // This is a "place"
      foo(P(ctx));  // This is a "place"
  }
  void main() {
      bar(P(ctx));  // This is a "place"
  }

  When you PACK a context, the current location (place+count) is pushed onto the path stack.
  When you UNPACK a context, a pop of that location is deferred to happen at the end of the scope.
                             THIS SHOULD BE DONE AT THE TOP OF THE PROCEDURES. Do not use the context
                             after the pop!
  
  IMPORTANT: When the UI_Context object is COPIED, we assume that it is passed by value to a procedure.
             So it is NOT ALLOWED to assign a UI_Context variable by copying a context by value.

 */



// PACK WITH COUNT
#define PC(CTX, COUNT)                          \
    CTX.pack(__COUNTER__+1, COUNT)

// PACK
#define P(CTX)                                  \
    PC(CTX, 0)

// UNPACK
#define U(CTX)                                  \
    CTX.set_state_to_unpacked();                \
    defer(pop_ui_location(CTX.manager);)


typedef u64 UI_ID;
const UI_ID NO_UI_ELEMENT = 0;

const u64 MAX_ID_PATH_LENGTH = 128; // 128 * 8B = 1024B

/*****
    STRUCTURE OF Location (u64)
    {
        u32 place; // This is a unique ID for the place in the code (__COUNTER__) IMPORTANT: Must be >0.
        u32 count;
    };
*****/
typedef u64 UI_Location;
struct UI_Path
{
    UI_Location e[MAX_ID_PATH_LENGTH];
};

struct UI_ID_Manager
{
    UI_ID last_id;

#if DEBUG
    Array<UI_ID, ALLOC_MALLOC> used_ids_this_build; // To detect if we generate the same ID for multiple elements. A build is a "frame"....
#endif
    
    struct Path_Bucket
    {
        //TODO @Incomplete: @Speed Make buckets here, where we store the paths after each other, terminating them with 64 zero bits.
        //                         We should have buckets because we don't want to copy all the paths when we run out of space.
        //                         Should be pretty big buckets though?

        Array<u64,     ALLOC_MALLOC> path_lengths;
        Array<UI_Path, ALLOC_MALLOC> paths;
        Array<UI_ID,   ALLOC_MALLOC> ids;
    };

    Path_Bucket path_buckets[256]; // Hash is 8 bits, and created from a Path.
    
};

struct UI_String
{
    u64 offset; // This is the offset from UI_Manager.string_data.e. IMPORTANT: UI_Manager.string_data is reset before every UI build, so make sure no invalid UI_Strings are stored!
    strlength length;
};

enum UI_Element_Type
{
    PANEL,
    WINDOW,
    BUTTON,
    TEXTFIELD,
    SLIDER,
    DROPDOWN,
    UI_TEXT,

    GRAPH,

    UI_INVENTORY_SLOT,
    UI_CHAT,
    
    UI_CHESS_BOARD,

#if DEBUG
    UI_PROFILER,
#endif
    
    WORLD_VIEW
};

enum UI_Click_State_
{
    IDLE                = 0b000000000000,
    HOVERED             = 0b000000000001,
    //HOVERED_NOW         = 0b000000000010,
    //UNHOVERED_NOW       = 0b000000000100, // This element was unhovered this frame
    PRESSED             = 0b000000001000, // State has been PRESSED_NOW and the mouse button is still down.
    CLICKED_AT_ALL      = 0b000000010000, // Clicked this frame.
    CLICKED_ENABLED     = 0b000000100000, // Clicked this frame AND the element was enabled.
    CLICKED_DISABLED    = 0b000001000000, // Clicked this frame AND the element was enabled.
    PRESSED_NOW         = 0b000010000000, // Pressed this frame.
    //RELEASED_NOW        = 0b000100000000, // It was pressed before, and it was released this frame.
    //MOUSE_UP_ON_NOW     = 0b001000000000, // The mouse was released this frame, and the cursor was over the element.
};
typedef u16 UI_Click_State; //In @JAI, this can probably be just an enum.

struct UI_Panel
{
    Rect a;
    v4 color;
};

struct UI_Window
{
    Rect initial_a;
    Rect current_a;
    v2 min_size;

    UI_String title;
    
    v4 border_color;
    v4 background_color;

    bool pressed;
    UI_Click_State close_button_state;
    bool has_close_button;

    s8 resize_dir_x;
    s8 resize_dir_y;
    bool moving;
    v2 mouse_offset_on_move_start;
    
    bool was_resized_or_moved; // This is set to true when a resize/move begins, and will remain false even if the user sets current_a back to initial_a.

    u32 num_children_above; // A window's children_above should always (after end_window()) be right before the window in UI_Manager.elements_in_depth_order.
};

struct UI_Text
{
    Rect a;
    
    UI_String text;

    v4 color;
    
    Font_Size font_size;
    Font_ID   font;

    H_Align h_align;
    V_Align v_align;
};

struct UI_Button
{
    Rect a;
    UI_Click_State state;

    UI_String label;
    
    bool enabled;
    bool selected;

    v4 color;
};

struct UI_Graph
{
    Rect a;
    UI_String data; // This is an array of floats. So, length % sizeof(float) must be 0...

    float y_min;
    float y_max;
};


struct UI_Inventory_Slot
{
    Rect a;
    UI_Click_State click_state;
    
    Item_Type_ID item_type;
    float fill;
    Inventory_Slot_Flags slot_flags;

    bool enabled;
    bool selected;
};

struct UI_Chat
{
    Rect a;
    UI_String text;
};

struct UI_Scrollbar
{
    UI_Click_State handle_click_state;
    float handle_grab_rel_p;
    
    float value;
};

struct UI_Textfield
{
    Rect a;
    UI_String text;

    UI_Click_State click_state;
    bool enabled;

    bool scrollbar_visible;
    UI_Scrollbar scroll;
};

struct UI_Slider
{
    Rect a;
    bool pressed;
    
    float value;

    bool enabled;
};

struct UI_Dropdown
{
    Rect box_a;
    UI_Click_State box_click_state;
    
    Array<UI_String, ALLOC_MALLOC> options;

    bool open;
};

void clear(UI_Dropdown *dropdown)
{
    clear(&dropdown->options);
}

struct UI_World_View
{
    Rect a;

    Camera camera;
    Ray mouse_ray;
    
    UI_Click_State click_state;

    s32 hovered_tile_ix; // -1 == no tile
    s32 pressed_tile_ix; // -1 == no tile
    s32 clicked_tile_ix; // -1 == no tile

    Entity_ID hovered_entity;
    bool entity_surface_hovered;
    v3   hovered_entity_hit_p;
    
    Entity_ID pressed_entity;
    Entity_ID clicked_entity;
};

struct UI_Chess_Board
{
    // These are set by the UI user code
    Rect a;
    UI_String board; // This is a Chess_Board, so length must be sizeof(Chess_Board). We have it as a string so we don't make UI_Element too big.

    s8 selected_square_ix; // -1 == no square.
    Chess_Move queued_move; // Set to = from to unset this.
    bool enabled;
    // --

    UI_Click_State click_state;

    s8 hovered_square_ix; // -1 == no square
    s8 pressed_square_ix; // -1 == no square
    s8 clicked_square_ix; // -1 == no square

};

#if DEBUG
struct UI_Profiler
{
    Rect a;

    int selection_start;
    int selection_end;
    int selected_frame;
    int selected_node;
};
#endif

struct UI_Element
{
    UI_Element_Type type;
    
    // @Speed: Put in its own array?
    bool needs_redraw;
    Rect last_dirty_rect; // The dirty rect that was used when we last drew this element.

    union {
        UI_Panel     panel;
        UI_Window    window;
        UI_Text      text;
        UI_Button    button;
        UI_Textfield textfield;
        UI_Slider    slider;
        UI_Dropdown  dropdown;

        UI_Graph graph;
        
        UI_Inventory_Slot inventory_slot;
        UI_Chat chat;

        UI_Chess_Board chess_board;

        UI_World_View world_view;

#if DEBUG
        UI_Profiler profiler;
#endif
    };
};

struct UI_Textfield_Caret
{
    u64 cp;
};

struct UI_Textfield_State
{
    UI_Textfield_Caret caret;
    UI_Textfield_Caret highlight_start;

    double last_scroll_to_caret_by_mouse_t;

    // NOTE: This is reset on textfield resize.
    Direction last_nav_dir;
    float last_vertical_nav_x; // Where the caret was on x (in pixels) before the last vertical navigation.
                               // This is relative to the textfield's text's position.

    float last_resize_w; // When a resize was last detected (or the textfield was marked active), this is the width the text area had.

    bool text_did_change; // This loop

    bool has_no_digits; // NOTE: This is for integer fields only.
    bool is_negative;   // NOTE: This is for integer fields only.
};

enum Color_Theme_ID {
    C_THEME_DEFAULT,
    C_THEME_MARKET,
    C_THEME_ROOM,

    C_THEME_NONE_OR_NUM
};

struct Color_Theme
{    
    v4 button;

    v4 panel;

    v4 window_border;
    v4 window_background;

    v4 text;
};

Color_Theme color_themes[] = {
    { C_BUTTON_DEFAULT, C_PANEL_DEFAULT, C_WINDOW_BORDER_DEFAULT, C_WINDOW_BACKGROUND_DEFAULT, C_TEXT_DEFAULT },
    { C_MARKET_BRIGHT,  C_MARKET_DARK,   C_MARKET_DARK,           C_MARKET_BASE,               C_WHITE },
    { C_ROOM_DARK,      C_ROOM_DARK,     C_ROOM_DARK,             C_ROOM_BASE,                 C_WHITE }
};
static_assert(ARRLEN(color_themes) == C_THEME_NONE_OR_NUM);

struct UI_Manager
{   
    UI_ID_Manager id_manager;

    // These are reset before every build //
    UI_Path current_path;
    u64     current_path_length;
    Array<Color_Theme_ID, ALLOC_MALLOC> color_theme_stack;
    // ////////////////////////////////// //

    Array<bool,       ALLOC_MALLOC> element_alives;
    Array<UI_ID,      ALLOC_MALLOC> element_ids;
    Array<UI_Element, ALLOC_MALLOC> elements;
    
    Array<UI_ID, ALLOC_MALLOC> window_stack;

    // These are reset before every build //
    Array<UI_ID, ALLOC_MALLOC> elements_in_depth_order; // TODO @Speed: After a build, find all elements by ID once and store the pointers in an array? That both the UI system and the renderer can use.
    Array<u8, ALLOC_MALLOC> string_data;
    // ////////////////////////////////// //
    Array<u8, ALLOC_MALLOC> last_string_data; // String data last build, so we can see if someone's data has changed between builds.

    UI_ID active_element;
    union {
        UI_Textfield_State active_textfield_state;
    };
};


#define _WINDOW___INTERNAL_(Ident, ...) \
    UI_ID Ident; \
    _AREA_(begin_window(&Ident, __VA_ARGS__)); \
    defer(end_window(Ident, ctx.manager);)

#define _WINDOW_(...) \
    _WINDOW___INTERNAL_(CONCAT(window_id_, __COUNTER__), __VA_ARGS__)


#define _THEME_(ThemeID) \
    array_add(ctx.manager->color_theme_stack, ThemeID); \
    defer(ctx.manager->color_theme_stack.n -= 1;)
    
