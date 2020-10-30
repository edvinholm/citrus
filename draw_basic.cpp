

//NOTE: n is number of vertices
void triangles_now(v3 *positions, /*v3 *normals,*/ v2 *uvs, v4 *colors, u64 n, Graphics *gfx)
{
    auto &set = gfx->vertex_shader.buffer_sets[gfx->buffer_set_index];
    
    gpu_set_vertex_buffer_data(set.position_buffer, positions, sizeof(v3) * n);
    gpu_set_vertex_buffer_data(set.uv_buffer,       uvs,       sizeof(v2) * n);
    gpu_set_vertex_buffer_data(set.color_buffer,    colors,    sizeof(v4) * n);
    
    gpu_draw(GPU_TRIANGLES, n);

    #if DEBUG
    gfx->debug.num_draw_calls++;
    #endif
}

inline
void triangles(v3 *p, v2 *uv, v4 *c, u32 n, Graphics *gfx)
{
    add_vertices(p, uv, c, n, &gfx->vertex_buffer);
    //triangles_now(vertices, uvs, colors, n, gfx);
}


//NOTE: _uvs should be (at least) 6*2 floats long (12 floats or 6 v2:s)
inline
void quad_uvs(v2 *_uvs, v2 uv0, v2 uv1)
{
    _uvs[0] = V2(uv0.x, uv0.y);
    _uvs[1] = V2(uv1.x, uv1.y);
    _uvs[2] = V2(uv0.x, uv1.y);
    
    _uvs[3] = V2(uv0.x, uv0.y);
    _uvs[4] = V2(uv1.x, uv1.y);
    _uvs[5] = V2(uv1.x, uv0.y);
}



void draw_quad_abs(v3 a, v3 b, v3 c, v3 d, Graphics *gfx, v2 *uvs = NULL)
{

    v3 v[6] = {
        a, d, c,
        a, d, b
    };

    /* @Normals: 
    v3 normal = normalized(-cross((b-a), (c-a)));

    v3 n[6] = {
        normal, normal, normal,
        normal, normal, normal
    };
    */
    
    const float default_uvs[6 * 2] = {
        0, 0,
        1, 1,
        0, 1,

        0, 0,
        1, 1,
        1, 0
    };

    v4 colors[6] = {
        gfx->current_color, gfx->current_color, gfx->current_color,
        gfx->current_color, gfx->current_color, gfx->current_color
    };

    if(uvs == NULL)
        uvs = (v2 *)default_uvs;

    triangles(v, uvs, colors, 6, gfx);
    
}


void draw_circle(v2 center, float radius, int num_slices, Graphics *gfx)
{
    const int slice_buffer_size = 36;
    const int vertices_per_slice = 3;
    const int num_vertices_in_slice_buffer = vertices_per_slice * 36;
    v3 vertices[num_vertices_in_slice_buffer];
    v4 colors  [num_vertices_in_slice_buffer];
    v2 uvs     [num_vertices_in_slice_buffer] = {0}; // Should we have Default Circle UVs?
    
    
    float angle_per_slice = (2 * PI)/num_slices;
    float a0 = 0;
    for(int s = 0; s < num_slices; s++)
    {
        float a1 = a0 + angle_per_slice;

        int buffer_slice_index = s % slice_buffer_size;

        v3 *vertex = &vertices[buffer_slice_index * vertices_per_slice];
        v4 *vertex_color =  &colors[buffer_slice_index * vertices_per_slice];
        
        *vertex = V3(center);
        *vertex_color = gfx->current_color;
        vertex++;
        vertex_color++;

        *vertex = V3(center + V2(cos(a0), sin(a0)) * radius);
        *vertex_color = gfx->current_color;
        vertex++;
        vertex_color++;

        *vertex = V3(center + V2(cos(a1), sin(a1)) * radius);
        *vertex_color = gfx->current_color;
        // Not needed: vertex++;

        if(buffer_slice_index == slice_buffer_size - 1)
        {
            triangles(vertices, uvs, colors, num_vertices_in_slice_buffer, gfx);
        }

        a0 = a1;
    }

    if(num_slices % slice_buffer_size > 0)
    {
        triangles(vertices, uvs, colors, (num_slices % slice_buffer_size) * vertices_per_slice, gfx);
    }
}


inline
void draw_quad(v3 p0, v3 d1, v3 d2, Graphics *gfx, v2 *uvs = NULL)
{
    v3 a = p0;
    v3 b = p0 + d1;
    v3 c = p0 + d2;
    v3 d = b + d2;
    draw_quad_abs(a, b, c, d, gfx, uvs);
}


void draw_triangle(v3 p0, v3 p1, v3 p2, Graphics *gfx, v2 *uvs = NULL)
{
    v3 v[3] = { p0, p1, p2 };
    
    v3 normal = normalized(-cross((p1 - p0), (p2 - p0)));
    v3 n[3] = { normal, normal, normal };

    v2 default_uvs[6] = {
        V2(0, 0),
        V2(1, 1),
        V2(0, 1)
    };

    v4 colors[6] = {
        gfx->current_color, gfx->current_color, gfx->current_color
    };

    if(uvs == NULL)
        uvs = default_uvs;

    triangles(v, uvs, colors, 3, gfx);
}

inline
void draw_triangle(v2 p0, v2 p1, v2 p2, Graphics *gfx, v2 *uvs = NULL)
{
    draw_triangle(V3(p0), V3(p1), V3(p2), gfx, uvs);
}

inline
void draw_rect_pp(v2 p0, v2 p1, Graphics *gfx, v2 *uvs = NULL)
{
    draw_quad_abs({p0.x, p0.y, 0}, {p1.x, p0.y, 0}, {p0.x, p1.y, 0}, {p1.x, p1.y, 0}, gfx, uvs);
}

inline
void draw_rect_d(v2 p0, v2 d1, v2 d2, Graphics *gfx, v2 *uvs = NULL)
{
    draw_quad(V3(p0), V3(d1), V3(d2), gfx, uvs);
}

inline
void draw_rect_ps(v2 p, v2 s, Graphics *gfx, v2 *uvs = NULL)
{
    v2 p1 = p + s;
    draw_rect_pp(p, p1, gfx, uvs);
}

inline
void draw_rect(Rect a, Graphics *gfx, v2 *uvs /* = NULL*/)
{
    return draw_rect_ps(a.p, a.s, gfx, uvs);
}
