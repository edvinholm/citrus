
m4x4 world_projection_matrix(Rect viewport)
{
    float x_mul, y_mul;
    if(viewport.w < viewport.h) {
        x_mul = 1;
        y_mul = 1.0f/(viewport.h / max(0.0001f, viewport.w));
    }
    else {
        y_mul = 1;
        x_mul = 1.0f/(viewport.w / max(0.0001f, viewport.h));
    }

    // TODO @Speed: @Cleanup: Combine matrices

    m4x4 world_projection = make_m4x4(
        x_mul, 0, 0, 0,
        0, y_mul, 0, 0,
        0, 0, -0.01, 0,
        0, 0, 0,     1);

    m4x4 rotation = rotation_matrix(axis_rotation(V3_X, PIx2 * 0.125));
    world_projection = matmul(rotation, world_projection);

    rotation = rotation_matrix(axis_rotation(V3_Z, PIx2 * -0.125));
    world_projection = matmul(rotation, world_projection);
    
    float diagonal_length = sqrt(room_size_x * room_size_x + room_size_y * room_size_y);
    
    m4x4 scale = scale_matrix(V3_ONE * (2.0 / diagonal_length));
    world_projection = matmul(scale, world_projection);
    
    m4x4 translation = translation_matrix({-(float)room_size_x/2.0, -(float)room_size_y/2.0, 0});
    world_projection = matmul(translation, world_projection);
    
    return world_projection;
}


Ray screen_point_to_ray(v2 p, Rect viewport, m4x4 projection_inverse)
{
    /* Translate to "-1 -> 1 space"...*/
    p -= viewport.p;
    p  = compdiv(p, viewport.s / 2.0f);
    p -= V2_XY;
    p.y *= -1;
    
    /* Make 3D vector */
    v3 u = { p.x, p.y, 0 };

    /* Unproject */
    Ray ray;
    ray.p0  = vecmatmul(u, projection_inverse);
    ray.dir = normalize(vecmatmul(V3_Z, projection_inverse));

    return ray;
}

#if 0
// no_checkin @Temporary
v3 ground_point_from_ray(Ray ray)
{
    if(is_zero(ray.z))
    {
        Assert(false);
        return V3_ZERO;
    }
    
    return p0 + (-p0.z / ray.z) * ray;
}
#endif
