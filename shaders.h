

struct Vertex_Shader
{
    GPU_Uniform_ID projection_uniform;
    GPU_Uniform_ID transform_uniform;

#if GFX_GL
    union {
        struct {
            GPU_Attribute_ID position_attr;
            GPU_Attribute_ID texcoord_attr;
            GPU_Attribute_ID color_attr;
        };
        GPU_Attribute_ID buffer_attributes[3];
    };
        
#endif

    //@Normals: GLint normal_attr;
    
    union
    {
        // IMPORTANT: There is some hardcoded stuff in gpu_set_buffer_set that needs to be updated if we add/remove/move buffers here. -EH, 2020-10-20
        struct Buffer_Set
        {
            union {
                struct {
                    GPU_Buffer_ID position_buffer;
                    GPU_Buffer_ID uv_buffer;
                    GPU_Buffer_ID color_buffer;
                };
                GPU_Buffer_ID buffers[3];
            };
        };
        
        Buffer_Set buffer_sets[2];
        GPU_Buffer_ID all_buffers[sizeof(buffer_sets)/sizeof(GPU_Buffer_ID)];
    };
};

struct Fragment_Shader
{
    GPU_Uniform_ID texture_uniform;
    GPU_Uniform_ID texture_present_uniform;

};

