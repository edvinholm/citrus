
inline
float eat_z_for_2d(Graphics *gfx)
{
    float z = gfx->z_for_2d;
    gfx->z_for_2d -= 0.0001f;
    return z;
}




inline
Vertex_Buffer<ALLOC_GFX> *current_vertex_buffer(Graphics *gfx)
{
    return current(gfx->vertex_buffer_stack, &gfx->default_vertex_buffer);
}




template<Allocator_ID A>
void ensure_capacity(Vertex_Buffer<A> *vb, u64 required_capacity)
{
    void **buffers[] = {
        (void **)&vb->p,
        (void **)&vb->uv,
        (void **)&vb->c,
        (void **)&vb->tex
    };

    size_t element_sizes[] = {
        sizeof(*vb->p),
        sizeof(*vb->uv),
        sizeof(*vb->c),
        sizeof(*vb->tex)
    };

    Assert(ARRLEN(buffers) == ARRLEN(element_sizes));

    ensure_buffer_set_capacity(required_capacity, &vb->capacity, buffers, element_sizes, ARRLEN(buffers), A);
}

template<Allocator_ID A>
void add_vertices(v3 *p, v2 *uv, v4 *c, float *tex, u64 n, Vertex_Buffer<A> *vb)
{
    ensure_capacity(vb, vb->n + n);

    Assert(sizeof(*p)  == sizeof(*vb->p));
    Assert(sizeof(*uv) == sizeof(*vb->uv));
    Assert(sizeof(*c)  == sizeof(*vb->c));
    Assert(sizeof(*tex)  == sizeof(*vb->tex));

    memcpy(vb->p   + vb->n, p,   sizeof(*p)   * n);
    memcpy(vb->uv  + vb->n, uv,  sizeof(*uv)  * n);
    memcpy(vb->c   + vb->n, c,   sizeof(*c)   * n);
    memcpy(vb->tex + vb->n, tex, sizeof(*tex) * n);

    vb->n += n;
}


GPU_Buffer_Set *current_default_buffer_set(Graphics *gfx)
{
    Assert(gfx->buffer_set_index < ARRLEN(gfx->vertex_shader.buffer_sets));
    Assert(gfx->buffer_set_index >= 0);
    
    return &gfx->vertex_shader.buffer_sets[gfx->buffer_set_index];
}


inline
VAO create_vao()
{
    VAO vao = {0};
    
    gpu_create_vao(&vao.id);
    gpu_create_buffers(ARRLEN(vao.buffer_set.buffers), vao.buffer_set.buffers);

    return vao;
}

inline
void bind_vao(VAO *vao, Graphics *gfx)
{
    gpu_bind_vao(vao->id);
    gpu_set_buffer_set(&vao->buffer_set, &gfx->vertex_shader);
}

inline
void unbind_vao(Graphics *gfx)
{
    auto *default_buffer_set = current_default_buffer_set(gfx);
    
    glBindVertexArray(0);
    gpu_set_buffer_set(default_buffer_set, &gfx->vertex_shader);
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


// NOTE: The vao contains a vertex0 and vertex1, which is used
//       to select the right vertices from the passed vertex_buffer.
template<Allocator_ID A>
void maybe_push_vao_to_gpu(VAO *vao, Vertex_Buffer<A> *vertex_buffer, Graphics *gfx)
{
    if(!vao->needs_push) return;
    
    Assert(vao->vertex0 >= 0);
    Assert(vao->vertex0 < vertex_buffer->n);
    
    Assert(vao->vertex1 >= vao->vertex1);
    Assert(vao->vertex1 <= vertex_buffer->n);

    auto *default_buffer_set = current_default_buffer_set(gfx);

    bind_vao(vao, gfx);
    {
        draw_vertex_buffer(vertex_buffer, false, &vao->buffer_set, vao->vertex0, vao->vertex1);
    }
    unbind_vao(gfx);
    
    vao->needs_push = false;
    Debug_Print("Pushed gfx.world.static_world_vao vertices to gpu.\n");
}
