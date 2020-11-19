

#include "draw_basic.cpp"




void flush_vertex_buffer(Vertex_Buffer *buffer, Graphics *gfx)
{
    triangles_now(buffer->p, buffer->uv, buffer->c, buffer->tex, buffer->n, gfx);
    buffer->n = 0;
}


inline
void flush_vertex_buffer(Graphics *gfx)
{
    flush_vertex_buffer(&gfx->vertex_buffer, gfx);
}
