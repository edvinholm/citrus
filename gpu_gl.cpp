

struct GPU_Context
{
    GLint shader_program;
};

inline
void gpu_set_vsync_enabled(bool enabled)
{
    wglSwapIntervalEXT((enabled) ? 1 : 0);
}

inline
bool gpu_init(float clear_color_r, float clear_color_g, float clear_color_b, float clear_color_a)
{
    if(!load_gl_extensions()) return false;
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_MULTISAMPLE);

    glClearColor(clear_color_r, clear_color_g, clear_color_b, clear_color_a);

    auto error = glGetError();
    return (error == 0);
}

inline
void gpu_set_depth_testing_enabled(bool enabled)
{
    if(enabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);  // @Robustness: Should be its own call.
    }
    else glDisable(GL_DEPTH_TEST);
}

inline
bool gpu_get_depth_mask()
{
    GLboolean mask;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &mask);
    return mask;
}

inline
void gpu_set_depth_mask(bool mask)
{
    glDepthMask(mask);
}

void gpu_set_buffer_set(GPU_Buffer_Set *set, Vertex_Shader *vertex_shader)
{
    for(int b = 0; b < ARRLEN(set->buffers); b++)
    {
        auto &buffer = set->buffers[b];
        glBindBuffer(GL_ARRAY_BUFFER, buffer); Assert(glGetError() == 0);

        // @Hack
        GLint element_size = 0;
        switch(b % 4) {
            
            case 0: { element_size = 3; } break;
            case 1: { element_size = 2; } break;
            case 2: { element_size = 4; } break;
            case 3: { element_size = 1; } break;

            default: Assert(false); break;
        }
        
        glVertexAttribPointer(vertex_shader->buffer_attributes[b], element_size, GL_FLOAT, GL_FALSE, 0, 0); { auto err = glGetError();  Assert(err == 0); }
        glEnableVertexAttribArray(vertex_shader->buffer_attributes[b]); Assert(glGetError() == 0);
        
        Assert(glGetError() == 0);
    }
}

inline
bool gpu_init_shaders(Vertex_Shader *vertex_shader, Fragment_Shader *fragment_shader,
                      GPU_Context *context)
{
    auto program = context->shader_program;
    
    vertex_shader->projection_uniform = glGetUniformLocation(program, "projection");
    vertex_shader->transform_uniform = glGetUniformLocation(program, "transform");
    
    //@ColorUniform: shader->color_uniform = glGetUniformLocation(program, "color");
    
    vertex_shader->position_attr = glGetAttribLocation(program, "aVertexPosition");
    vertex_shader->texcoord_attr = glGetAttribLocation(program, "aTexCoord");
    vertex_shader->color_attr = glGetAttribLocation(program, "color");
    vertex_shader->texture_attr = glGetAttribLocation(program, "texture");
    //@Normals: shader->normal_attr = glGetAttribLocation(program, "aNormal");


    /* @Normals:
      Uncomment this when the shader actually uses the value - otherwise it might be optimized out from the shader.
      
    glBindBuffer(GL_ARRAY_BUFFER, gfx.normal_buffer);
    glVertexAttribPointer(shader->normal_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(shader->normal_attr);
    */


    // GENERATE BUFFERS //
    glGenBuffers(ARRLEN(vertex_shader->all_buffers), vertex_shader->all_buffers);
    Assert(glGetError() == 0);

    gpu_set_buffer_set(&vertex_shader->buffer_sets[0], vertex_shader); // Don't know if this is a good idea to set a buffer set from the beginning.
    // //////////////// //
        
    fragment_shader->texture_1_uniform = glGetUniformLocation(program, "texture_1");
    fragment_shader->texture_2_uniform = glGetUniformLocation(program, "texture_2");
    fragment_shader->texture_3_uniform = glGetUniformLocation(program, "texture_3");
    fragment_shader->texture_4_uniform = glGetUniformLocation(program, "texture_4");

    return (glGetError()) ? false : true;
}

inline
void gpu_clear_depth_buffer()
{
    Assert(gpu_get_depth_mask() == GL_TRUE); // Stupid
    glClear(GL_DEPTH_BUFFER_BIT);
}

inline
void gpu_set_uniform_m4x4(GPU_Uniform_ID uniform, m4x4 m)
{
    glUniformMatrix4fv(uniform, 1, GL_FALSE, m.elements);
}


inline
void gpu_set_uniform_int(GPU_Uniform_ID uniform, int i)
{
    glUniform1i(uniform, i);
}

inline
void gpu_bind_texture(GPU_Texture_ID tex, int active_texture = 0)
{
    glActiveTexture(GL_TEXTURE0 + active_texture);
    glBindTexture(GL_TEXTURE_2D, tex);
}


inline
bool gpu_compile_shaders(u8 *vertex_shader_code, u8 *fragment_shader_code,
                         GPU_Context *context)
{
    if(!create_gl_program(vertex_shader_code, fragment_shader_code, &context->shader_program))
        return false;
    
    glUseProgram(context->shader_program);

    auto gl_error = glGetError();
    Release_Assert_Args(gl_error == 0,    "gl_error", gl_error);
    
    return gl_error == 0;
}

// NOTE: You need to do a gpu_ensure_buffer_size before calling this.
inline
void gpu_set_vertex_buffer_data(GPU_Buffer_ID buffer, void *data, size_t size, bool static_data)
{
    Assert(data != NULL);
    Assert(size >= 0);
    
	{ auto err = glGetError(); Assert(err == 0); }
    glBindBuffer(GL_ARRAY_BUFFER, buffer);                      { auto err = glGetError(); Assert(err == 0); }
    glBufferData(GL_ARRAY_BUFFER, size, data, (static_data) ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW); { auto err = glGetError(); Assert(err == 0); }
}


inline
void gpu_draw(GPU_Primitive_Type type, int num_vertices, size_t offset = 0)
{
    Assert(num_vertices >= 0);
    glDrawArrays(type, offset, num_vertices);

    Assert(glGetError() == 0);
}


inline
void gpu_set_scissor(float x, float y, float w, float h)
{
    glScissor(x, y, w, h);
    glEnable(GL_SCISSOR_TEST);
}

inline
void gpu_disable_scissor()
{
    glDisable(GL_SCISSOR_TEST);
}

inline
void gpu_set_viewport(float x, float y, float w, float h)
{
    glViewport(x, y, w, h);
}

//TODO @Robustness: Pass min/mag filters
inline
bool gpu_create_texture(u32 w, u32 h, GPU_Texture_Parameters params, GPU_Texture_ID *_id, GPU_Error_Code *_error_code = NULL)
{
    //TODO @Incomplete: Use params
    
    glGenTextures(1, _id);
    
    glBindTexture(GL_TEXTURE_2D, *_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    auto err = glGetError();
    if(_error_code) *_error_code = err;
    return (err == 0);
}


inline
void gpu_create_framebuffers(int n, GPU_Framebuffer_ID *_ids)
{
    glGenFramebuffers(n, _ids);
}

inline
void gpu_update_framebuffer(GPU_Framebuffer_ID id, GPU_Texture_ID color_attachment0 = 0, bool color_attachment0_is_multisample = false, GPU_Texture_ID depth_attachment = 0)
{
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    if(color_attachment0 != 0) {
        auto texture_slot = (color_attachment0_is_multisample) ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture_slot, color_attachment0, 0);
    }

    if(depth_attachment != 0) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, depth_attachment, 0);
    }
    
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    // @Norelease TODO @Robustness: This assert fails when we minimize the game window.....!!!!
    // @Norelease TODO @Robustness: This assert fails when we minimize the game window.....!!!!
    // @Norelease TODO @Robustness: This assert fails when we minimize the game window.....!!!!
    // @Norelease TODO @Robustness: This assert fails when we minimize the game window.....!!!!
    Assert(status == GL_FRAMEBUFFER_COMPLETE);
}

inline
int gpu_max_num_multisample_samples()
{
    int max_samples;
    glGetIntegerv (GL_MAX_SAMPLES, &max_samples);
    Assert(glGetError() == 0);
    return max_samples;
}

//IMPORTANT: id=0 means the texture does not exist, so we create one. This works for OpenGL, since 0 is not a valid texture ID.
//           But we need to make sure this is true in other GPU APIs as well! @Robustness
inline
bool gpu_update_or_create_multisample_texture(GPU_Texture_ID *id, u32 width, u32 height, int num_samples, GPU_Error_Code *_error_code)
{
    if(*id == 0) {
        glGenTextures(1, id);
    }            
    
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, *id);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, num_samples, GL_RGBA8,
                            width, height, GL_FALSE);
    auto err = glGetError();
    Assert(err == 0);

    *_error_code = glGetError();
    return (*_error_code == 0);    
}


//IMPORTANT: id=0 means the texture does not exist, so we create one. This works for OpenGL, since 0 is not a valid texture ID.
//           But we need to make sure this is true in other GPU APIs as well! @Robustness
inline
bool gpu_update_or_create_multisample_depth_buffer_texture(GPU_Texture_ID *id, u32 width, u32 height, int num_samples, GPU_Error_Code *_error_code)
{
    if(*id == 0) {
        glGenTextures(1, id);
    }
    
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, *id);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, num_samples, GL_DEPTH_COMPONENT,
                            width, height, GL_FALSE);
    
    auto err = glGetError();
    Assert(err == 0);

    *_error_code = glGetError();
    return (*_error_code == 0);    
}

inline
GPU_Texture_Parameters gpu_texture_parameters(GPU_Texture_Pixel_Components pixel_components,
                                              GPU_Texture_Pixel_Format     pixel_format,
                                              GPU_Texture_Pixel_Data_Type  pixel_data_type)
{
    GPU_Texture_Parameters params;
    Zero(params);
    params.pixel_components   = pixel_components;
    params.pixel_format = pixel_format;
    params.pixel_data_type    = pixel_data_type;

    return params;
}

inline
void gpu_set_texture_data(GPU_Texture_ID texture, void *data, u32 w, u32 h,
                          GPU_Texture_Parameters params)
{
    Assert(texture != 0);

    GLint old_bound;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_bound);
    {
        glBindTexture(GL_TEXTURE_2D, texture);
        auto gl_error = glGetError();
        Assert(gl_error == 0);
        
        glTexImage2D(GL_TEXTURE_2D, 0, params.pixel_components, w, h, 0,
                     params.pixel_format, params.pixel_data_type, data);
        
        gl_error = glGetError();
        Assert(gl_error == 0);
    }
    glBindTexture(GL_TEXTURE_2D, old_bound);
}


inline
void gpu_create_buffers(int num_buffers, GPU_Buffer_ID *_buffers)
{
    glGenBuffers(num_buffers, _buffers);
}

inline
void gpu_create_vao(GPU_Vertex_Array_ID *_id)
{
    glGenVertexArrays(1, _id);
}

inline
void gpu_bind_vao(GPU_Vertex_Array_ID id)
{
    glBindVertexArray(id);
}

inline
void gpu_unbind_vao()
{
    glBindVertexArray(0);
}
