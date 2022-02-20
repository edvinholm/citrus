

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
#define _OPAQUE_UI_VERTEX_OBJECT_(Transform_Matrix, Screen_Z, Scissor)          \
    _VERTEX_OBJECT_(VD_UI_OPAQUE, &gfx->ui_render_buffer.opaque, Transform_Matrix, 0, C_WHITE, Scissor)

#define _TRANSLUCENT_UI_VERTEX_OBJECT_(Transform_Matrix, Screen_Z, Scissor)     \
    _VERTEX_OBJECT_(VD_UI_TRANSLUCENT, &gfx->ui_render_buffer.translucent, Transform_Matrix, Screen_Z, C_WHITE, Scissor)

#define _OPAQUE_UI_() \
    _OPAQUE_UI_VERTEX_OBJECT_(M_IDENTITY, gfx->z_for_2d, (gfx->scissor.size > 0) ? opt(current(gfx->scissor)) : opt_empty<Rect>())

#define _TRANSLUCENT_UI_() \
    _TRANSLUCENT_UI_VERTEX_OBJECT_(M_IDENTITY, gfx->z_for_2d, (gfx->scissor.size > 0) ? opt(current(gfx->scissor)) : opt_empty<Rect>())



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
            
        case VD_PREVIEWS_OPAQUE:      buffer = &gfx->previews_render_buffer.opaque.vertices; break;
        case VD_PREVIEWS_TRANSLUCENT: buffer = &gfx->previews_render_buffer.translucent.vertices; break;
            
        default: Assert(false); break;
    }

    push(gfx->vertex_buffer, buffer);
}

void pop_vertex_destination(Graphics *gfx)
{
    pop(gfx->vertex_buffer);
}


void begin_vertex_render_object(Render_Object_Buffer *buffer, m4x4 transform, float screen_z = 0, v4 color = C_WHITE, Optional<Rect> scissor = {0})
{
    Assert(!buffer->current_vertex_object_began);

    auto &obj = buffer->current_vertex_object;
    Zero(obj);
    
    obj.type  = VERTEX_OBJECT;
    
    obj.screen_z  = screen_z;
    
    obj.transform = transform;
    obj.color     = color;

    obj.scissor = scissor;
    
    obj.vertex0   = buffer->vertices.n;

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
    
    triangles_now(buffer->p       + vertex0,
                  buffer->uv      + vertex0,
                  buffer->c       + vertex0,
                  buffer->tex     + vertex0,
                  buffer->normals + vertex0,
                  vertex1 - vertex0,
                  buffer_set,
                  do_dynamic_draw_now);
}


template<Allocator_ID A>
void reset_vertex_buffer(Vertex_Buffer<A> *buffer)
{
    buffer->n = 0;
}



inline
void draw_vao(VAO *vao, Graphics *gfx)
{
    bind_vao(vao, gfx);
    {
        gpu_draw(GPU_TRIANGLES, vao->vertex1 - vao->vertex0, 0);
    }
    unbind_vao(gfx);
}

void set_gpu_properties_for_render_object(Render_Object *obj, Graphics *gfx)
{
    if(obj->scissor.present) {
        auto sc = obj->scissor.value;
        gpu_set_scissor(sc.x, sc.y, sc.w, sc.h);
    } else {
        gpu_disable_scissor();
    }
    
    auto *vs = &gfx->vertex_shader;
    gpu_set_uniform_m4x4 (vs->transform_uniform,        obj->transform);
    gpu_set_uniform_v4   (vs->color_multiplier_uniform, obj->color);
    gpu_set_uniform_float(vs->desaturation_uniform,     obj->desaturation);

    auto *fs = &gfx->fragment_shader;
    gpu_set_uniform_v3(fs->lightbox_center_uniform,   obj->lightbox_center);
    gpu_set_uniform_v3(fs->lightbox_radiuses_uniform, obj->lightbox_radiuses);
    gpu_set_uniform_v4(fs->lightbox_color_uniform,    obj->lightbox_color);
    gpu_set_uniform_int(fs->do_edge_detection_uniform, obj->do_edge_detection);
}

template<Allocator_ID A>
void draw_vertex_render_object(Render_Object *obj, Vertex_Buffer<A> *vertex_buffer, bool do_dynamic_draw_now, GPU_Buffer_Set *buffer_set, Graphics *gfx)
{
    Assert(obj->type == VERTEX_OBJECT);

    set_gpu_properties_for_render_object(obj, gfx);
    
    auto vertex0 = obj->vertex0;
    auto num_vertices = (obj->vertex1 - vertex0);
    
    triangles_now(vertex_buffer->p       + vertex0,
                  vertex_buffer->uv      + vertex0,
                  vertex_buffer->c       + vertex0,
                  vertex_buffer->tex     + vertex0,
                  vertex_buffer->normals + vertex0,
                  num_vertices, buffer_set,
                  do_dynamic_draw_now);
}


void draw_render_object(Render_Object *obj, Render_Object_Buffer *object_buffer, bool do_dynamic_draw_now, GPU_Buffer_Set *buffer_set, Graphics *gfx)
{
    switch(obj->type) { // @Jai #complete
        case VERTEX_OBJECT: {
            draw_vertex_render_object(obj, &object_buffer->vertices, do_dynamic_draw_now, buffer_set, gfx);
        } break;

        case MESH_OBJECT: {
            set_gpu_properties_for_render_object(obj, gfx);
            draw_vao(obj->mesh_vao, gfx);            
        } break;

        default: Assert(false); break;
    }
    
}

void reset_render_object_buffer(Render_Object_Buffer *buffer)
{
    reset(&buffer->vertices);
    buffer->objects.n = 0;
}

bool render_object_properties_equal(Render_Object *a, Render_Object *b)
{
    // @Speed.....
    if(a->transform != b->transform) return false;
    if(a->color     != b->color)     return false;
    if(a->scissor   != b->scissor)   return false;
    
    if(!floats_equal(a->desaturation, b->desaturation)) return false;
    
    if(a->lightbox_center   != b->lightbox_center)   return false;
    if(a->lightbox_radiuses != b->lightbox_radiuses) return false;
    if(a->lightbox_color    != b->lightbox_color)    return false;

    if(a->do_edge_detection != b->do_edge_detection) return false;

    return true;
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
        
        int obj_ix = 0;
        Vertex_Buffer<ALLOC_MALLOC> temporary_vertex_buffer = {0}; // IMPORTANT: Do not use temporary memory here. We cannot use that when we don't have the mutex locked. (@Speed: Make one temp mem per thread)
        ensure_capacity(&temporary_vertex_buffer, vertices.n);
        defer(clear(&temporary_vertex_buffer););

        while(obj_ix < num_objects) {

            if(buffer->objects[object_indices[obj_ix]].type == VERTEX_OBJECT) {
                reset(&temporary_vertex_buffer);
                
                // NOTE: Here we try to combine vertex objects that can be drawn with
                //       one draw call (where we don't need to change uniforms etc).
                //       Combinable objectscts must come right after one another though,
                //       because we still want to draw them in the order we've sorted
                //       them in.

                int first_ix = obj_ix;
                
                Render_Object temporary_object = buffer->objects[object_indices[first_ix]];
                
                temporary_object.vertex0 = 0;
                temporary_object.vertex1 = 0;

                while(obj_ix < num_objects)
                {
                    auto *obj = &buffer->objects[object_indices[obj_ix]];

                    if(obj->type != VERTEX_OBJECT) break;                
                    if(obj_ix > first_ix) {
                        if(!render_object_properties_equal(obj, &temporary_object)) break;
                    }
               
                    auto v0 = obj->vertex0;
                    auto num_vertices = obj->vertex1 - v0;
                    add_vertices(vertices.p + v0, vertices.uv + v0, vertices.c + v0, vertices.tex + v0, vertices.normals + v0, num_vertices, &temporary_vertex_buffer);
                    temporary_object.vertex1 += num_vertices;

                    obj_ix++;
                }
                
                draw_vertex_render_object(&temporary_object, &temporary_vertex_buffer, true, current_default_buffer_set(gfx), gfx);
            }
            else {
                draw_render_object(&buffer->objects[object_indices[obj_ix]], buffer, true, current_default_buffer_set(gfx), gfx);
                obj_ix++;
            }
            
        }
        
    }
    else
    {
        // DRAW UNSORTED //
        // Tries to combine objects with the same transform.
        
        int obj_ix = 0;
        while(obj_ix < buffer->objects.n)
        {
            if(buffer->objects[obj_ix].type == VERTEX_OBJECT)
            {
                int first_ix = obj_ix;
                Render_Object temporary_object = buffer->objects[first_ix];

                while(obj_ix < buffer->objects.n)
                {
                    auto *obj = &buffer->objects[obj_ix];
                    
                    if(obj->type != VERTEX_OBJECT) break;
                    if(obj_ix > first_ix) {
                        if(obj->vertex0 != temporary_object.vertex1) break;
                        if(!render_object_properties_equal(obj, &temporary_object)) break;
                    }

                    temporary_object.vertex1 = obj->vertex1;
                
                    obj_ix++;
                }
            
                draw_vertex_render_object(&temporary_object, &vertices, true, current_default_buffer_set(gfx), gfx);
            }
            else {
                draw_render_object(&buffer->objects[obj_ix], buffer, true, current_default_buffer_set(gfx), gfx);
                obj_ix++;
            }
        }
    }
}

Render_Object *draw_mesh(VAO *mesh_vao, m4x4 transform, Render_Object_Buffer *object_buffer, Graphics *gfx, float screen_z = 0, v4 color = C_WHITE)
{
    Assert(!object_buffer->current_vertex_object_began);
    
    Render_Object obj = {0};
    obj.type      = MESH_OBJECT;
    
    obj.transform = transform;
    obj.color     = color;
    
    obj.mesh_vao  = mesh_vao;
    obj.screen_z  = screen_z;

    return array_add(object_buffer->objects, obj);
}

Render_Object *draw_mesh(Mesh_ID mesh, m4x4 transform, Render_Object_Buffer *object_buffer, Graphics *gfx, float screen_z = 0, v4 color = C_WHITE) {
    Assert(mesh >= 0 && mesh < ARRLEN(gfx->assets->mesh_vaos));
    return draw_mesh(&gfx->assets->mesh_vaos[mesh], transform , object_buffer, gfx, screen_z, color);
}
