

#define _VERTEX_DESTINATION_(Destination)    \
    push_vertex_destination(Destination, gfx);     \
    defer(pop_vertex_destination(gfx););

// NOTE: Use _OPAQUE_WORLD_VERTEX_OBJECT_ or _TRANSLUCENT_WORLD_VERTEX_OBJECT_ etc if you can.
#define _VERTEX_OBJECT_(Vertex_Destination, Buffer_Ptr, ...)    \
    _VERTEX_DESTINATION_(Vertex_Destination); \
    begin_vertex_render_object(Buffer_Ptr, __VA_ARGS__); \
    defer(end_vertex_render_object(Buffer_Ptr););

// WORLD
#define _OPAQUE_WORLD_VERTEX_OBJECT_(Transform_Matrix)                 \
    _VERTEX_OBJECT_(VD_WORLD_OPAQUE, &gfx->world_render_buffer.opaque, Transform_Matrix, 0)

#define _TRANSLUCENT_WORLD_VERTEX_OBJECT_(Transform_Matrix, Screen_Z) \
    _VERTEX_OBJECT_(VD_WORLD_TRANSLUCENT, &gfx->world_render_buffer.translucent, Transform_Matrix, Screen_Z)

// UI
#define _OPAQUE_UI_VERTEX_OBJECT_(Transform_Matrix, Screen_Z)                 \
    _VERTEX_OBJECT_(VD_UI_OPAQUE, &gfx->ui_render_buffer.opaque, Transform_Matrix, 0)

#define _TRANSLUCENT_UI_VERTEX_OBJECT_(Transform_Matrix, Screen_Z) \
    _VERTEX_OBJECT_(VD_UI_TRANSLUCENT, &gfx->ui_render_buffer.translucent, Transform_Matrix, Screen_Z)

#define _OPAQUE_UI_() \
    _OPAQUE_UI_VERTEX_OBJECT_(M_IDENTITY, gfx->z_for_2d)

#define _TRANSLUCENT_UI_() \
    _TRANSLUCENT_UI_VERTEX_OBJECT_(M_IDENTITY, gfx->z_for_2d)



#include "draw_basic.cpp"


void push_vertex_destination(Vertex_Destination dest, Graphics *gfx)
{
    auto *buffer = &gfx->default_vertex_buffer;
    
    switch(dest) {
        case VD_DEFAULT: break;
            
        case VD_WORLD_OPAQUE:      buffer = &gfx->world_render_buffer.opaque.vertices; break;
        case VD_WORLD_TRANSLUCENT: buffer = &gfx->world_render_buffer.translucent.vertices; break;

        case VD_UI_OPAQUE:      buffer = &gfx->ui_render_buffer.opaque.vertices; break;
        case VD_UI_TRANSLUCENT: buffer = &gfx->ui_render_buffer.translucent.vertices; break;
            
        default: Assert(false); break;
    }

    push(gfx->vertex_buffer, buffer);
}

void pop_vertex_destination(Graphics *gfx)
{
    pop(gfx->vertex_buffer);
}


void begin_vertex_render_object(Render_Object_Buffer *buffer, m4x4 transform, float screen_z = 0)
{
    Assert(!buffer->current_vertex_object_began);

    auto &obj = buffer->current_vertex_object;
    obj.type      = VERTEX_OBJECT;
    obj.transform = transform;
    obj.vertex0   = buffer->vertices.n;
    obj.screen_z  = screen_z;
    
    buffer->current_vertex_object_began = true;
}

void end_vertex_render_object(Render_Object_Buffer *buffer)
{
    Assert(buffer->current_vertex_object_began);
    buffer->current_vertex_object_began = false;
    
    Assert(buffer->vertices.n >= 0);

    if(buffer->vertices.n == 0) return;
    
    auto &obj = buffer->current_vertex_object;
    
//    Assert(buffer->vertices.n > obj.vertex0);
    obj.vertex1 = buffer->vertices.n;

    array_add(buffer->objects, obj);
}

template<Allocator_ID A>
void draw_vertex_buffer(Vertex_Buffer<A> *buffer, bool do_dynamic_draw_now, GPU_Buffer_Set *buffer_set, u64 vertex0 = 0, u64 vertex1 = U64_MAX)
{
    vertex1 = min(buffer->n, vertex1);
    Assert(vertex0 < vertex1);

    triangles_now(buffer->p   + vertex0,
                  buffer->uv  + vertex0,
                  buffer->c   + vertex0,
                  buffer->tex + vertex0,
                  vertex1 - vertex0,
                  buffer_set,
                  do_dynamic_draw_now);
}


template<Allocator_ID A>
void reset_vertex_buffer(Vertex_Buffer<A> *buffer)
{
    buffer->n = 0;
}



template<Allocator_ID A>
void draw_render_object(Render_Object *obj, Vertex_Buffer<A> vertices, bool do_dynamic_draw_now, GPU_Buffer_Set *buffer_set, Graphics *gfx)
{
    auto vertex0 = obj->vertex0;
    auto num_vertices = (obj->vertex1 - vertex0);

    gpu_set_uniform_m4x4(gfx->vertex_shader.transform_uniform, obj->transform);

    triangles_now(vertices.p   + vertex0,
                  vertices.uv  + vertex0,
                  vertices.c   + vertex0,
                  vertices.tex + vertex0,
                  num_vertices, buffer_set,
                  do_dynamic_draw_now);
}

void reset_render_object_buffer(Render_Object_Buffer *buffer)
{
    reset(&buffer->vertices);
    buffer->objects.n = 0;
}

// IMPORTANT: We cannot use temporary memory in this proc since we call it when the mutex is unlocked.
void draw_render_object_buffer(Render_Object_Buffer *buffer, bool do_sort, Graphics *gfx)
{
    const Allocator_ID allocator = ALLOC_MALLOC;
    
    Assert(!buffer->current_vertex_object_began);

    if (buffer->objects.n == 0) return;

    // NOTE: We copy the whole Vertex_Buffer struct here. This is for @Speed, but not good for @Robustness if we are not careful.
    auto vertices = buffer->vertices;

    if(do_sort) {

        // SORT OBJECTS //
        auto num_objects = buffer->objects.n;
        
        int *object_indices = (int *)alloc(sizeof(int) * num_objects, allocator); // @Speed
        defer(dealloc(object_indices, allocator););

        // NOTE: These values don't move around when we sort.
        //       An object's z does not necessarily live at the
        //       same index in object_z as its index does in
        //       object_indices.
        float *object_z = (float *)alloc(sizeof(float) * num_objects, allocator); // @Speed
        defer(dealloc(object_z, allocator););
        
        // @Speed
        for(int o = 0; o < num_objects; o++) {
            object_indices[o] = o;
            object_z[o]       = buffer->objects[o].screen_z; // @Speed: screen_z should be store in a separate array already.
        }

#if DEBUG
        for (int o = 0; o < num_objects; o++) {
            Assert(object_indices[o] >= 0 && object_indices[o] < buffer->objects.n);
        }
#endif

        // @Speed @Speed @Speed: This is a bubblesort.
        int at = 1;
        while(at < buffer->objects.n)
        {
            int ix = at;
            while(ix > 0 && object_z[ix] > object_z[ix-1])
            {
                swap(object_indices + ix-1, object_indices + ix);
                swap(object_z       + ix-1, object_z       + ix);
                ix--;
            }
            at++;
        }

#if DEBUG
        for (int o = 0; o < num_objects; o++) {
            Assert(object_indices[o] >= 0 && object_indices[o] < buffer->objects.n);
        }
#endif

        // DRAW //
        // NOTE: Here we try to combine objects that can be drawn with
        //       one draw call (where we don't need to change uniforms etc).
        //       Combinable objects must come right after one another though,
        //       because we still want to draw them in the order we've sorted
        //       them in.
        
        int obj_ix = 0;
        Vertex_Buffer<ALLOC_MALLOC> temporary_vertex_buffer = {0}; // IMPORTANT: Do not use temporary memory here. We cannot use that when we don't have the mutex locked. (@Speed: Make one temp mem per thread)
        ensure_capacity(&temporary_vertex_buffer, vertices.n);
        defer(clear(&temporary_vertex_buffer););

        while(obj_ix < num_objects) {

            int first_ix = obj_ix;
            
            Render_Object temporary_object = buffer->objects[object_indices[first_ix]];
            Assert(temporary_object.type == VERTEX_OBJECT);
            
            temporary_object.vertex0 = 0;
            temporary_object.vertex1 = 0;

            while(obj_ix < num_objects)
            {
                auto *obj = &buffer->objects[object_indices[obj_ix]];

                Assert(obj->type == VERTEX_OBJECT);
                
                if(obj_ix > first_ix && obj->transform != temporary_object.transform) break;
               
                auto v0 = obj->vertex0;
                auto num_vertices = obj->vertex1 - v0;
                add_vertices(vertices.p + v0, vertices.uv + v0, vertices.c + v0, vertices.tex + v0, num_vertices, &temporary_vertex_buffer);
                temporary_object.vertex1 += num_vertices;

                obj_ix++;
            }
            
            draw_render_object(&temporary_object, temporary_vertex_buffer, true, current_default_buffer_set(gfx), gfx);
            reset(&temporary_vertex_buffer);
        }
        
    }
    else
    {
        // DRAW UNSORTED //
        // Tries to combine objects with the same transform.
        
        int obj_ix = 0;
        while(obj_ix < buffer->objects.n) {

            int first_ix = obj_ix;
            Render_Object temporary_object = buffer->objects[first_ix];

            while(obj_ix < buffer->objects.n)
            {
                auto *obj = &buffer->objects[obj_ix];

                Assert(obj->type == VERTEX_OBJECT);

                if(obj_ix > first_ix && (obj->transform != temporary_object.transform ||
                                         obj->vertex0 != temporary_object.vertex1)) break;

                temporary_object.vertex1 = obj->vertex1;
                
                obj_ix++;
            }
            
            draw_render_object(&temporary_object, vertices, true, current_default_buffer_set(gfx), gfx);
        }
    }
}
