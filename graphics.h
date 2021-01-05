

struct Graphics_Debug
{
    u64 num_draw_calls;
    
    String_Builder sb;
};


template<Allocator_ID A>
struct Vertex_Buffer
{
    u64 capacity;
    u64 n;

    v3    *p;   // positions
    v2    *uv;  // texture coordinates
    v4    *c;   // colors
    float *tex; // textures
};

template<Allocator_ID A>
void reset(Vertex_Buffer<A> *buffer)
{
    buffer->n = 0;
}

template<Allocator_ID A>
void clear(Vertex_Buffer<A> *buffer)
{
    if(buffer->p)
        dealloc_buffer_set(buffer->p, A);
}


struct VAO
{
    GPU_Vertex_Array_ID id;
    GPU_Buffer_Set buffer_set;

    
    bool needs_push; // if true, we should push the updated vertices to the gpu.

    // Absolute values of vertex0 and vertex1 are only valid
    // if needs_push is true. BUT, vertex1-vertex0 should always
    // give the number of vertices the vao consists of.
    u64 vertex0; // In the vertex buffer we used. (Usually gfx.universal_vertex_buffer)
    u64 vertex1;
};



// @Incomplete: This is supposed to determine if we should give the gpu a mesh ID or vertices...
enum Render_Object_Type
{
    VERTEX_OBJECT
    // @Incomplete: MESH_OBJECT    or something..
};

struct Render_Object
{
    Render_Object_Type type;
    float screen_z;
    m4x4 transform;

    union {
        struct { // VERTEX_OBJECT
            u64 vertex0;
            u64 vertex1;
        };
    };
};

// @Incomplete: :VertexObjectBufferIncomplete Sort objects by screen z before rendering them.
struct Render_Object_Buffer
{
    bool current_vertex_object_began;
    Render_Object current_vertex_object;
    
    Array<Render_Object, ALLOC_GFX> objects;
    Vertex_Buffer<ALLOC_GFX> vertices;
};

void reset(Render_Object_Buffer *buffer)
{
    buffer->objects.n = 0;
    reset(&buffer->vertices);
}

struct World_Render_Buffer
{
    Render_Object_Buffer opaque;
    Render_Object_Buffer translucent;
};
void reset(World_Render_Buffer *buffer)
{
    reset(&buffer->opaque);
    reset(&buffer->translucent);
}

struct UI_Render_Buffer
{
    Render_Object_Buffer opaque;
    Render_Object_Buffer translucent;
};
void reset(UI_Render_Buffer *buffer)
{
    reset(&buffer->opaque);
    reset(&buffer->translucent);
}


enum Vertex_Destination
{
    VD_DEFAULT          = 0,
    
    VD_WORLD_OPAQUE,
    VD_WORLD_TRANSLUCENT,

    VD_UI_OPAQUE,
    VD_UI_TRANSLUCENT
};

// TODO @Cleanup: Move
template<typename T, int Max>
struct Static_Stack
{
    T e[Max];
    int size;
};

template<typename T, int Max>
void push(Static_Stack<T, Max> &stack, T elem)
{
    Assert(stack.size < Max);
    Assert(stack.size >= 0);
    stack.e[stack.size++] = elem;
}

template<typename T, int Max>
T pop(Static_Stack<T, Max> &stack)
{
    Assert(stack.size <= Max);
    Assert(stack.size > 0);
    return stack.e[--stack.size];
}

template<typename T, int Max>
T current(Static_Stack<T, Max> &stack, T default_if_empty)
{
    Assert(stack.size <= Max);
    Assert(stack.size >= 0);
    if(stack.size == 0) return default_if_empty;
    else return stack.e[stack.size-1];
}

struct World_Graphics
{
    bool static_opaque_vao_up_to_date;
    VAO  static_opaque_vao; // For things that doesn't change very often. Like floors and walls.
};

struct Graphics
{
    GPU_Context gpu_ctx;

    // Buffered draw data
    Vertex_Buffer<ALLOC_GFX> default_vertex_buffer;
    Vertex_Buffer<ALLOC_GFX> universal_vertex_buffer;
    World_Render_Buffer world_render_buffer;
    UI_Render_Buffer    ui_render_buffer;

    // Draw context
    v2 frame_s;

    // State
    u8 buffer_set_index;
    Static_Stack<Vertex_Buffer<ALLOC_GFX> *, 8> vertex_buffer_stack; // IMPORTANT: Don't set this directly. Use push_vertex_destination().
    
    float   z_for_2d; // This is the Z value that will be set for "2D vertices"
                      // NOTE: Use eat_z_for_2d() to get the copy current value and then decrease the original, so that the next thing you draw have a smaller z.
    
    v4      current_color;
    Font_ID current_font;

    // GPU resources
    GPU_Texture_ID     multisample_texture;
    GPU_Texture_ID     depth_buffer_texture;
    GPU_Framebuffer_ID framebuffer;
    
    Vertex_Shader   vertex_shader;
    Fragment_Shader fragment_shader;

    // "Assets"
    Texture_Catalog textures;
    Texture_ID bound_textures[4]; // In fragment shader
    u8 num_bound_textures;
    
    Sprite_Map glyph_maps[NUM_FONTS];
    Font *fonts; // IMPORTANT: Graphics does not own this memory. This needs to be NUM_FONTS long.

    // World
    World_Graphics world;

    // Debug
#if DEBUG
    Graphics_Debug debug;
#endif
};
