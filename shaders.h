
struct Vertex_Shader
{
    GPU_Uniform_ID projection_uniform;
    GPU_Uniform_ID transform_uniform;

#if GFX_GL
    GPU_Attribute_ID vertex_position_attr;
    GPU_Attribute_ID texcoord_attr;
    GPU_Attribute_ID color_attr;
#endif

    //@Normals: GLint normal_attr;
    
    union
    {
        struct
        {
            GPU_Buffer_ID vertex_buffer;
            GPU_Buffer_ID uv_buffer;
            GPU_Buffer_ID color_buffer;
        };
        GPU_Buffer_ID buffers[6];
    };
};

struct Fragment_Shader
{
    GPU_Uniform_ID texture_uniform;
    GPU_Uniform_ID texture_present_uniform;

};

