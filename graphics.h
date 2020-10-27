

struct Graphics
{
    GPU_Context gpu_ctx;

    GPU_Texture_ID multisample_texture;
    GLuint multisample_framebuffer; // nocheckin
    
    u8 buffer_set_index;
    
    Vertex_Shader   vertex_shader;
    Fragment_Shader fragment_shader;

    v2 frame_s;
};
