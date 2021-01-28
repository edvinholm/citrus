
struct Camera
{
    m4x4 projection;
    m4x4 projection_inverse;
};


m4x4 world_projection_matrix(Rect viewport, float zoffs = 0);
