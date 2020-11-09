
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
    Array<UI_ID, ALLOC_UI> used_ids_this_build; // To detect if we generate the same ID for multiple elements. A build is a "frame"....
#endif
    
    struct Path_Bucket
    {
        //TODO @Incomplete: @Speed Make buckets here, where we store the paths after each other, terminating them with 64 zero bits.
        //                         We should have buckets because we don't want to copy all the paths when we run out of space.
        //                         Should be pretty big buckets though?

        Array<u64,     ALLOC_UI> path_lengths;
        Array<UI_Path, ALLOC_UI> paths;
        Array<UI_ID,   ALLOC_UI> ids;
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
    WINDOW,
    BUTTON,
    SLIDER,
    DROPDOWN
};

enum UI_Button_State_
{
    IDLE                = 0b000000000000,
    HOVERED             = 0b000000000001,
    //HOVERED_NOW         = 0b000000000010,
    //UNHOVERED_NOW       = 0b000000000100, // This element was unhovered this frame
    PRESSED             = 0b000000001000, // State has been PRESSED_NOW and the mouse button is still down.
    CLICKED_AT_ALL      = 0b000000010000, // Clicked this frame.
    CLICKED_ENABLED     = 0b000000100000, // Clicked this frame AND the element was enabled.
    CLICKED_DISABLED    = 0b000001000000, // Clicked this frame AND the element was enabled.
    //PRESSED_NOW         = 0b000010000000, // Pressed this frame.
    //RELEASED_NOW        = 0b000100000000, // It was pressed before, and it was released this frame.
    //MOUSE_UP_ON_NOW     = 0b001000000000, // The mouse was released this frame, and the cursor was over the element.
};
typedef u16 UI_Button_State; //In @JAI, this can probably be just an enum.

struct UI_Window
{
    Rect initial_a;
    Rect current_a;

    UI_String title;
    
    bool pressed;
    UI_Button_State close_button_state;
    bool has_close_button;

    s8 resize_dir_x;
    s8 resize_dir_y;
    bool moving;
    v2 mouse_offset_on_move_start;
    
    bool was_resized_or_moved; // This is set to true when a resize/move begins, and will remain false even if the user sets current_a back to initial_a.

    u32 num_children_above; // A window's children_above should always (after end_window()) be right before the window in UI_Manager.elements_in_depth_order.
};

struct UI_Button
{
    Rect a;
    UI_Button_State state;

    UI_String label;
    
    bool disabled;
    bool selected;
};

struct UI_Slider
{
    Rect a;
    bool pressed;
    
    float value;

    bool disabled;
};

struct UI_Dropdown
{
    Rect box_a;
    UI_Button_State box_button_state;
    
    Array<UI_String, ALLOC_UI> options;

    bool open;
};

void clear(UI_Dropdown *dropdown)
{
    clear(&dropdown->options);
}


struct UI_Element
{
    UI_Element_Type type;

    union {
        UI_Window window;
        UI_Button button;
        UI_Slider slider;
        UI_Dropdown dropdown;
    };
};

struct UI_Manager
{   
    UI_ID_Manager id_manager;

    UI_Path current_path;
    u64     current_path_length;

    Array<bool,       ALLOC_UI> element_alives;
    Array<UI_ID,      ALLOC_UI> element_ids;
    Array<UI_Element, ALLOC_UI> elements;
    
    Array<UI_ID, ALLOC_UI> window_stack;

    // These are reset before every build //
    Array<UI_ID, ALLOC_UI> elements_in_depth_order; // TODO @Speed: After a build, find all elements by ID once and store the pointers in an array? That both the UI system and the renderer can use.
    Array<u8, ALLOC_UI> string_data;
    // ////////////////////////////////// //
};
