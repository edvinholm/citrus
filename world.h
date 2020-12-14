
struct Camera
{
    m4x4 projection;
    m4x4 projection_inverse;
};

struct Ray
{
    v3 p0;
    v3 dir;
};


m4x4 world_projection_matrix(Rect viewport);
