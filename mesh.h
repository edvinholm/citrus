
struct Mesh
{   
    v3 *positions;
    v3 *normals;
    v2 *uvs;

    v4 *colors;

    // Filled in when we draw... @Speed
    float *tex;
    
    int n;
};

void clear(Mesh *mesh, Allocator_ID allocator) {
    dealloc_buffer_set(mesh->positions, allocator);
}

void ensure_mesh_capacity(u64 required_capacity, u64 *capacity, Mesh *mesh, Allocator *allocator)
{    
    // @Jai: auto-generate.
    void **buffers[] = {
        (void **)&mesh->positions,
        (void **)&mesh->normals,
        (void **)&mesh->uvs,
        (void **)&mesh->colors,
        (void **)&mesh->tex
    };

    // @Jai: auto-generate.
    size_t buffer_element_sizes[] = {
        sizeof(*mesh->positions),
        sizeof(*mesh->normals),
        sizeof(*mesh->uvs),
        sizeof(*mesh->colors),
        sizeof(*mesh->tex)
    };

    static_assert(ARRLEN(buffer_element_sizes) == ARRLEN(buffers));
    
    ensure_buffer_set_capacity(required_capacity, capacity, buffers, buffer_element_sizes, ARRLEN(buffers), allocator);
}

Mesh create_mesh(v3 *positions, v3 *normals, v2 *uvs, int num_vertices, Allocator *allocator)
{
    Mesh mesh = {0};
    
    static_assert(sizeof(*positions) == sizeof(*mesh.positions));
    static_assert(sizeof(*normals)   == sizeof(*mesh.normals));
    static_assert(sizeof(*uvs)       == sizeof(*mesh.uvs));
    
    u64 capacity = 0; // We don't need to store this unless we want to have 'dynamic' meshes that can grow.
    ensure_mesh_capacity(num_vertices, &capacity, &mesh, allocator);

    // @Jai: auto-generate.
    memcpy(mesh.positions, positions, sizeof(*mesh.positions) * num_vertices);
    memcpy(mesh.normals,   normals,   sizeof(*mesh.normals)   * num_vertices);
    memcpy(mesh.uvs,       uvs,       sizeof(*mesh.uvs)       * num_vertices);

    for(int i = 0; i < num_vertices; i++) {
        mesh.colors[i] = { 1, 1, 1, 1 };
    }

    memset(mesh.tex, 0, sizeof(*mesh.tex) * num_vertices);

    mesh.n = num_vertices;
    
    return mesh;
}

Mesh copy_mesh(Mesh *mesh, Allocator *allocator)
{
    return create_mesh(mesh->positions, mesh->normals, mesh->uvs, mesh->n, allocator);
}



bool ray_intersects_mesh(Ray ray, Mesh *mesh, v3 *_intersection, float *_ray_t)
{
    Function_Profile();
    
    bool  any_hit = false;
    float closest_hit_t = FLT_MAX;
    v3    closest_hit;
    
    int i = 0;
    while(i < mesh->n)
    {
        v3 a = mesh->positions[i++];
        v3 b = mesh->positions[i++];
        v3 c = mesh->positions[i++];

        v4 plane = triangle_plane(a, b, c);

        v3    intersection;
        float ray_t;
        if(!ray_intersects_plane(ray, plane, &intersection, &ray_t)) continue;
        if(any_hit && ray_t >= closest_hit_t) continue;

        auto bc = barycentric(intersection, a, b, c);
        if(bc.x >= 0 && bc.y >= 0 && bc.z >= 0) {
            any_hit = true;
            closest_hit   = intersection;
            closest_hit_t = ray_t;
        }
    }

    if(!any_hit) return false;

    *_intersection = closest_hit;
    *_ray_t        = closest_hit_t;
    return true;
}
