 



void ensure_capacity(Vertex_Buffer *vb, u64 required_capacity)
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

    ensure_buffer_set_capacity(required_capacity, &vb->capacity, buffers, element_sizes, ARRLEN(buffers), ALLOC_GFX);
}

void add_vertices(v3 *p, v2 *uv, v4 *c, float *tex, u64 n, Vertex_Buffer *vb)
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

