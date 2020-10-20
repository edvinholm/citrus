

struct Graphics
{
    u8 buffer_set_index;
    
    GPU_Context gpu_ctx;
    
    Vertex_Shader   vertex_shader;
    Fragment_Shader fragment_shader;

    v2 frame_s;
};
