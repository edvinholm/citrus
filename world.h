
struct Camera
{
    m4x4 projection;
    m4x4 projection_inverse;
};


m4x4 world_projection_matrix(v2 viewport_s, float room_sx, float room_sy, float room_sz, float z_offset = 0, bool apply_tweaks = true);
