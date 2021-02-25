
#define Scoped_Profile___(Ident, ...)                     \
    push_profiler_node(PROFILER, __VA_ARGS__);  \
    defer({ \
        pop_profiler_node(PROFILER); \
    })

#define Scoped_Profile__(Counter, ...) \
    Scoped_Profile___(CONCAT(__did_push_profiler_node_, Counter), __VA_ARGS__)

#define Scoped_Profile_(...) \
    Scoped_Profile__(__COUNTER__, __VA_ARGS__)

#define Scoped_Profile(...) \
    Scoped_Profile_(__VA_ARGS__)


#define Function_Profile() \
    Scoped_Profile_(__FUNCTION__);

    

const u8 MAX_PROFILER_NODE_CHILDREN = 16;

struct Profiler_Node
{
    Profiler_Node *parent;
    s64 t0; // This is a value from QueryPerformanceCounter (on win32).
    s64 t1; // This is a value from QueryPerformanceCounter (on win32).

    Profiler_Node *children[MAX_PROFILER_NODE_CHILDREN];
    u8 num_children;

    s32   user_int;
    float user_float;
    
    const char *label;
};

const s32 MAX_PROFILER_FRAME_NODES = 1024;

struct Profiler_Frame
{
    Profiler_Node nodes [MAX_PROFILER_FRAME_NODES];
    s32 num_nodes;

    Profiler_Node *current_node;
};

struct Profiler
{
    Profiler_Frame frames[2048];
    u64            frame_cursor;

    bool paused;
};


// @Jai: Put in context.
Profiler *PROFILER = (Profiler *)calloc(1, sizeof(Profiler));


void next_profiler_frame(Profiler *profiler);

void push_profiler_node(Profiler *profiler, const char *label);

Profiler_Node *pop_profiler_node(Profiler *profiler, s32 user_int = 0, float user_float = 0);
