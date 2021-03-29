
struct GPU_Buffer_Set
{
    union {
        struct {
            GPU_Buffer_ID position_buffer;
            GPU_Buffer_ID uv_buffer;
            GPU_Buffer_ID color_buffer;
            GPU_Buffer_ID texture_buffer;
            GPU_Buffer_ID normal_buffer;
        };
        GPU_Buffer_ID buffers[5];
    };
};



struct Vertex_Shader
{
    GPU_Uniform_ID projection_uniform;
    GPU_Uniform_ID transform_uniform;
    
    GPU_Uniform_ID mode_2d_uniform;
    GPU_Uniform_ID color_multiplier_uniform;

#if GFX_GL
    union {
        struct {
            GPU_Attribute_ID position_attr;
            GPU_Attribute_ID texcoord_attr;
            GPU_Attribute_ID color_attr;
            GPU_Attribute_ID texture_attr;
            GPU_Attribute_ID normal_attr;
        };
        GPU_Attribute_ID buffer_attributes[5];
    };
        
#endif
    
    union
    {
        // IMPORTANT: There is some hardcoded stuff in gpu_set_buffer_set that needs to be updated if we add/remove/move buffers here. -EH, 2020-10-20
        
        GPU_Buffer_Set buffer_sets[2];
    };
};

struct Fragment_Shader
{
    GPU_Uniform_ID texture_1_uniform;
    GPU_Uniform_ID texture_2_uniform;
    GPU_Uniform_ID texture_3_uniform;
    GPU_Uniform_ID texture_4_uniform;

    GPU_Uniform_ID lightbox_center_uniform;
    GPU_Uniform_ID lightbox_radiuses_uniform;
    GPU_Uniform_ID lightbox_color_uniform;

    GPU_Uniform_ID do_edge_detection_uniform;
};

