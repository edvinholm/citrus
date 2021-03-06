

//NOTE: n is number of vertices
void triangles_now(v3 *positions, v2 *uvs, v4 *colors, float *textures, v3 *normals, u64 n, GPU_Buffer_Set *set, bool do_dynamic_draw_now)
{
    bool static_draw = !do_dynamic_draw_now;
    
    gpu_set_vertex_buffer_data(set->position_buffer, positions, sizeof(v3) * n,    static_draw);
    gpu_set_vertex_buffer_data(set->uv_buffer,       uvs,       sizeof(v2) * n,    static_draw);
    gpu_set_vertex_buffer_data(set->color_buffer,    colors,    sizeof(v4) * n,    static_draw);
    gpu_set_vertex_buffer_data(set->texture_buffer,  textures,  sizeof(float) * n, static_draw);
    gpu_set_vertex_buffer_data(set->normal_buffer,   normals,   sizeof(v3) * n,    static_draw);

    if(do_dynamic_draw_now) gpu_draw(GPU_TRIANGLES, n);
}


void triangles(v3 *p, v2 *uv, v4 *c, float *tex, v3 *normals, u32 n, Graphics *gfx)
{
    auto *buffer = current_vertex_buffer(gfx);

    if(!buffer) {
        triangles_now(p, uv, c, tex, normals, n, current_default_buffer_set(gfx), true);
        return;
    }
    
    Assert(buffer);
    add_vertices(p, uv, c, tex, normals, n, buffer);

    // MULTIPLY BY CURRENT TRANSFORM MATRIX //
    if(gfx->transform.size > 0)
    {
        m4x4 transform = current(gfx->transform);
        
        auto *added_p = buffer->p + buffer->n - n;
        
        for(u32 i = 0; i < n; i++) {
            v3 v = added_p[i];
            added_p[i] = vecmatmul(v, transform);
        }
    }
}

template<u32 NUM_VERTICES>
void draw_polygon(v3 *p, v4 color, Graphics *gfx, v2 *uvs = NULL, Texture_ID texture = TEX_NONE_OR_NUM, v3 *normals = NULL)
{        
    v4 colors[NUM_VERTICES];
    for(int i = 0; i < ARRLEN(colors); i++) colors[i] = color;

    v2 default_uvs[NUM_VERTICES] = {0};
    if(!uvs) uvs = default_uvs;

    // @Speed @Ugly
    // @Speed @Ugly
    // @Speed @Ugly
    static v3 default_normals[NUM_VERTICES] = {0};
    static bool default_normals_initialized = false;
    if(!normals) {
        if(!default_normals_initialized) {
            for(int i = 0; i < NUM_VERTICES; i++) {
                default_normals[i] = V3_Z;
            }
            default_normals_initialized = true;
        }
        normals = default_normals;
    }

    float textures[NUM_VERTICES];
    float t = bound_slot_for_texture(texture, gfx);
    for(int i = 0; i < ARRLEN(colors); i++) textures[i] = t;

    triangles(p, uvs, colors, textures, normals, NUM_VERTICES, gfx);
}


//NOTE: _uvs should be (at least) 6*2 floats long (12 floats or 6 v2:s)

void quad_uvs(v2 *_uvs, v2 uv0, v2 uv1)
{
    _uvs[0] = { uv0.x, uv0.y };
    _uvs[1] = { uv1.x, uv1.y };
    _uvs[2] = { uv0.x, uv1.y };
    
    _uvs[3] = { uv0.x, uv0.y };
    _uvs[4] = { uv1.x, uv1.y };
    _uvs[5] = { uv1.x, uv0.y };
}



void draw_quad_abs(v3 a, v3 b, v3 c, v3 d, v4 color, Graphics *gfx, v2 *uvs = NULL, Texture_ID texture = TEX_NONE_OR_NUM)
{
    v3 v[6] = {
        a, d, c,
        a, d, b
    };

    v3 normal = normalize(cross((b-a), (c-a)));

    v3 normals[6] = {
        normal, normal, normal,
        normal, normal, normal
    };
    
    const float default_uvs[6 * 2] = {
        0, 0,
        1, 1,
        0, 1,

        0, 0,
        1, 1,
        1, 0
    };

    v4 colors[6] = {
        color, color, color,
        color, color, color
    };

    float t = bound_slot_for_texture(texture, gfx);

    float tex[6] = {
        t, t, t,
        t, t, t
    };

    if(uvs == NULL)
        uvs = (v2 *)default_uvs;

    triangles(v, uvs, colors, tex, normals, 6, gfx);
}


void draw_cube_ps(v3 p0, v3 s, v4 color, Graphics *gfx, v2 *uvs = NULL, Texture_ID texture = TEX_NONE_OR_NUM)
{
    v3 p1 = p0 + s;

    v3 a = { p0.x, p0.y, p0.z };
    v3 b = { p0.x, p0.y, p1.z };
    v3 c = { p1.x, p0.y, p0.z };

    v3 d = { p1.x, p1.y, p1.z };
    v3 e = { p1.x, p1.y, p0.z };
    v3 f = { p1.x, p0.y, p1.z };

    v3 g = { p0.x, p1.y, p1.z };
    v3 h = { p0.x, p1.y, p0.z };
    
    v3 v[6*6] = {
        
        // Towards negative X
        a, g, b,
        a, g, h,

        // Towards positive X
        e, c, d,
        f, c, d,
        
        // Towards negative Y
        a, f, b,
        a, c, f,

        // Towards positive Y
        h, d, e,
        h, g, d,

        // Towards negative Z
        e, h, a,
        e, a, c,

        // Towards positive Z
        b, f, d,
        b, d, g

    };
    
    
    const float default_uvs[6*6*2] = {
        0, 0,
        1, 1,
        0, 1,
        0, 0,
        1, 1,
        1, 0,

        0, 0,
        1, 1,
        1, 0,
        0, 0,
        1, 1,
        0, 1,

        0, 0,
        1, 1,
        0, 1,
        0, 0,
        1, 1,
        1, 0,

        
        1, 0,
        0, 0,
        1, 1,
        0, 1,
        0, 0,
        1, 1,

        0, 0,
        1, 1,
        0, 1,
        0, 0,
        1, 1,
        1, 0,

        1, 0,
        0, 0,
        1, 1,
        0, 1,
        0, 0,
        1, 1
    };
    
    if(uvs == NULL)
        uvs = (v2 *)default_uvs;

    v4 colors[6*6] = {
        color, color, color,
        color, color, color,
        
        color, color, color,
        color, color, color,
        
        color, color, color,
        color, color, color,
        
        color, color, color,
        color, color, color,
        
        color, color, color,
        color, color, color,
        
        color, color, color,
        color, color, color
    };

    float t = bound_slot_for_texture(texture, gfx);

    float tex[6*6] = {
        t, t, t,
        t, t, t,
        
        t, t, t,
        t, t, t,
        
        t, t, t,
        t, t, t,
        
        t, t, t,
        t, t, t,
        
        t, t, t,
        t, t, t,
        
        t, t, t,
        t, t, t
    };

    v3 normals[6*6] = {
        -V3_X, -V3_X, -V3_X,
        -V3_X, -V3_X, -V3_X,

         V3_X, V3_X, V3_X,
         V3_X, V3_X, V3_X,
        
        -V3_Y, -V3_Y, -V3_Y,
        -V3_Y, -V3_Y, -V3_Y,

         V3_Y, V3_Y, V3_Y,
         V3_Y, V3_Y, V3_Y,

        -V3_Z, -V3_Z, -V3_Z,
        -V3_Z, -V3_Z, -V3_Z,

         V3_Z, V3_Z, V3_Z,
         V3_Z, V3_Z, V3_Z
    };

    triangles(v, uvs, colors, tex, normals, 6*6, gfx);
}

#if 0
// See @Incomplete notes in func.
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
            // @Incomplete: Pass texture!!!
            triangles(vertices, uvs, colors, 0, num_vertices_in_slice_buffer, gfx);
        }

        a0 = a1;
    }

    if(num_slices % slice_buffer_size > 0)
    {
        // @Incomplete: Pass texture!!!
        triangles(vertices, uvs, colors, 0, (num_slices % slice_buffer_size) * vertices_per_slice, gfx);
    }
}
#endif


// @Speed
// @Speed
// @Speed

void draw_quad(v3 p0, v3 d1, v3 d2, v4 color, Graphics *gfx, v2 *uvs = NULL, Texture_ID tex = TEX_NONE_OR_NUM)
{
    v3 b = p0 + d1;
    v3 c = p0 + d2;
    v3 d = b + d2;
    draw_quad_abs(p0, b, c, d, color, gfx, uvs, tex);
}

void draw_quad(Quad a, v4 color, Graphics *gfx, v2 *uvs = NULL, Texture_ID tex = TEX_NONE_OR_NUM)
{    
    v3 d1 = rotate_vector(V3_X, a.q);
    v3 d2 = rotate_vector(V3_Y, a.q);
    
    draw_quad(a.p, d1, d2, color, gfx, uvs, tex);
}

// NOTE: n is the normal.
void draw_line(v3 a, v3 b, v3 n, float w, v4 color, Graphics *gfx, v2 *uvs = NULL, Texture_ID tex = TEX_NONE_OR_NUM)
{
    v3 ortho = normalize(cross(b - a, n));
    v3 p0 = a - ortho * w * 0.5f;
    
    draw_quad(p0, ortho * w, (b - a), color, gfx, uvs, tex);
}

void draw_line(v2 a, v2 b, float w, v4 color, Graphics *gfx, float z_3d = 0, v2 *uvs = NULL, Texture_ID tex = TEX_NONE_OR_NUM)
{
    Draw_Mode draw_mode = current(gfx->draw_mode);
    
    float z = 0;
    if     (draw_mode == DRAW_2D) z = eat_z_for_2d(gfx);
    else if(draw_mode == DRAW_3D) z = z_3d;

    draw_line({ a.x, a.y, z }, { b.x, b.y, z }, V3_Z, w, color, gfx, uvs, tex);
}

#if 0
// @Incomplete!! Can't pass null as tex to triangles()
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

    triangles(v, uvs, colors, 0, 3, gfx);
}

void draw_triangle(v2 p0, v2 p1, v2 p2, Graphics *gfx, v2 *uvs = NULL)
{
    draw_triangle(V3(p0), V3(p1), V3(p2), gfx, uvs);
}

#endif

void draw_rect(Rect a, v4 color, Graphics *gfx, float z_3d = 0, v2 *uvs = NULL, Texture_ID texture = TEX_NONE_OR_NUM)
{
    Draw_Mode draw_mode = current(gfx->draw_mode);

    float z = 0;
    if     (draw_mode == DRAW_2D) z = eat_z_for_2d(gfx);
    else if(draw_mode == DRAW_3D) z = z_3d;
    
    
    v3 v[6] = {
        { a.x,       a.y,       z },
        { a.x,       a.y + a.h, z },
        { a.x + a.w, a.y,       z },

        { a.x + a.w, a.y,       z },
        { a.x,       a.y + a.h, z },
        { a.x + a.w, a.y + a.h, z }
    };

    v2 default_uvs[6] = {
        0, 0,
        0, 1,
        1, 0,

        1, 0,
        0, 1,
        1, 1
    };

    if(!uvs)
        uvs = default_uvs;
    
    v4 c[6] = {
        color,
        color,
        color,
        
        color,
        color,
        color
    };


    float t = 0;

    if(texture != TEX_NONE_OR_NUM)
    {
        Assert(gfx->num_bound_textures > texture && gfx->bound_textures[texture] == texture);    
        t = (float)texture+1;
    }
             
    float tex[6] = {
        t, t, t,
        t, t, t
    };

    v3 normals[6] = {
        V3_Z, V3_Z, V3_Z,
        V3_Z, V3_Z, V3_Z
    };

    triangles(v, uvs, c, tex, normals, 6, gfx);
}

void draw_rect_pp(v2 p0, v2 p1, v4 color, Graphics *gfx, float z = 0, v2 *uvs = NULL, Texture_ID texture = TEX_NONE_OR_NUM)
{
    draw_rect({p0, (p1 - p0)}, color, gfx, z, uvs, texture);
}

void draw_rect_ps(v2 p0, v2 s, v4 color, Graphics *gfx, float z = 0, v2 *uvs = NULL, Texture_ID texture = TEX_NONE_OR_NUM)
{
    draw_rect({p0, s}, color, gfx, z, uvs, texture);
}
