
struct Mesh
{   
    v3 *positions;
    v3 *normals;
    v2 *uvs;

    v4 *colors;

    // Filled in when we draw... @Speed
    float *tex;
    
    int n;
    
    v3 *real_triangle_normals; // Length of this is n/3.
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

    u64 new_capacity = *capacity;
    ensure_buffer_set_capacity(required_capacity, &new_capacity, buffers, buffer_element_sizes, ARRLEN(buffers), allocator);

    // Real Triangle Normals //
    Assert(required_capacity % 3 == 0);
    u64 triangle_capacity = *capacity / 3;
    ensure_capacity(&mesh->real_triangle_normals, &triangle_capacity, required_capacity/3, allocator);
    // // //

    *capacity = new_capacity;
}

Mesh create_mesh(v3 *positions, v3 *normals, v2 *uvs, v4 *colors, int num_vertices, Allocator *allocator)
{
    Mesh mesh = {0};
    
    static_assert(sizeof(*positions) == sizeof(*mesh.positions));
    static_assert(sizeof(*normals)   == sizeof(*mesh.normals));
    static_assert(sizeof(*uvs)       == sizeof(*mesh.uvs));
    static_assert(sizeof(*colors)    == sizeof(*mesh.colors));
    
    u64 capacity = 0; // We don't need to store this unless we want to have 'dynamic' meshes that can grow.
    ensure_mesh_capacity(num_vertices, &capacity, &mesh, allocator);

    // @Jai: auto-generate.
    memcpy(mesh.positions, positions, sizeof(*mesh.positions) * num_vertices);
    memcpy(mesh.normals,   normals,   sizeof(*mesh.normals)   * num_vertices);
    memcpy(mesh.uvs,       uvs,       sizeof(*mesh.uvs)       * num_vertices);
    memcpy(mesh.colors,    colors,    sizeof(*mesh.colors)    * num_vertices);

    memset(mesh.tex, 0, sizeof(*mesh.tex) * num_vertices);

    for(int t = 0; t < num_vertices/3; t++) {
        v3 a = mesh.positions[t * 3 + 0];
        v3 b = mesh.positions[t * 3 + 1];
        v3 c = mesh.positions[t * 3 + 2];

        mesh.real_triangle_normals[t] = normalize(cross((b-a), (c-a)));
    }

    mesh.n = num_vertices;
    
    return mesh;
}

Mesh copy_mesh(Mesh *mesh, Allocator *allocator)
{
    return create_mesh(mesh->positions, mesh->normals, mesh->uvs, mesh->colors, mesh->n, allocator);
}



bool ray_intersects_mesh(Ray ray, Mesh *mesh, v3 *_intersection, float *_ray_t)
{
    Function_Profile();
    
    bool  any_hit = false;
    float closest_hit_t = FLT_MAX;
    v3    closest_hit;
    
    int t = -1;
    int num_triangles = mesh->n/3;
    while(++t < num_triangles)
    {
        v3 normal = mesh->real_triangle_normals[t];
        
        if(dot(normal, ray.dir) >= 0) continue;
        
        v3 a = mesh->positions[t*3+0];
        v3 b = mesh->positions[t*3+1];
        v3 c = mesh->positions[t*3+2];

        float plane_d = -dot(a, normal);
        
        float ray_t     = (-plane_d - normal.x*ray.p0.x - normal.y*ray.p0.y - normal.z*ray.p0.z) / (normal.x*ray.dir.x + normal.y*ray.dir.y + normal.z*ray.dir.z);
        v3 intersection = ray.p0 + ray.dir * ray_t;
        
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
