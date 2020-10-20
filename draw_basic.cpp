

//NOTE: n is number of vertices
void triangles_now(v3 *positions, /*v3 *normals,*/ v2 *uvs, v4 *colors, u64 n, Graphics *gfx)
{
    // nocheckin: Alternate between buffers.

    auto &set = gfx->vertex_shader.buffer_sets[gfx->buffer_set_index];
    
    gpu_set_vertex_buffer_data(set.position_buffer, positions, sizeof(v3) * n);
    gpu_set_vertex_buffer_data(set.uv_buffer,       uvs,       sizeof(v2) * n);
    gpu_set_vertex_buffer_data(set.color_buffer,    colors,    sizeof(v4) * n);
    
    gpu_draw(GPU_TRIANGLES, n);
}
