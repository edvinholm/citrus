
struct Graphics
{
    GPU_Context gpu_ctx;

    GPU_Texture_ID     multisample_texture;
    GPU_Framebuffer_ID multisample_framebuffer;
    
    u8 buffer_set_index;
    
    Vertex_Shader   vertex_shader;
    Fragment_Shader fragment_shader;

    v2 frame_s;

    Texture_Catalog textures;
    
    Sprite_Map glyph_maps[NUM_FONTS];
    Font       fonts[NUM_FONTS] = {0};

    v4      current_color;
    Font_ID current_font;
};
