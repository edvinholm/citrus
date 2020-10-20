
struct GPU_Context
{
    GLint shader_program;
};

inline
bool gpu_init(float clear_color_r, float clear_color_g, float clear_color_b, float clear_color_a)
{
    if(!load_gl_extensions()) return false;
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(clear_color_r, clear_color_g, clear_color_b, clear_color_a);

    auto error = glGetError();
    return (error == 0);
}

inline
bool gpu_init_shaders(Vertex_Shader *vertex_shader, Fragment_Shader *fragment_shader,
                      GPU_Context *context)
{
    auto program = context->shader_program;
    
    vertex_shader->projection_uniform = glGetUniformLocation(program, "projection");
    vertex_shader->transform_uniform = glGetUniformLocation(program, "transform");
    
    //@ColorUniform: shader->color_uniform = glGetUniformLocation(program, "color");
    
    vertex_shader->vertex_position_attr = glGetAttribLocation(program, "aVertexPosition");
    vertex_shader->texcoord_attr = glGetAttribLocation(program, "aTexCoord");
    vertex_shader->color_attr = glGetAttribLocation(program, "color");
    //@Normals: shader->normal_attr = glGetAttribLocation(program, "aNormal");


    /* @Normals:
      Uncomment this when the shader actually uses the value - otherwise it might be optimized out from the shader.
      
    glBindBuffer(GL_ARRAY_BUFFER, gfx.normal_buffer);
    glVertexAttribPointer(shader->normal_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(shader->normal_attr);
    */

    Zero(vertex_shader->uv_buffer);
    glGenBuffers(1, &vertex_shader->uv_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_shader->uv_buffer);
    glVertexAttribPointer(vertex_shader->texcoord_attr, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertex_shader->texcoord_attr);

    Zero(vertex_shader->vertex_buffer);
    glGenBuffers(1, &vertex_shader->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_shader->vertex_buffer);
    glVertexAttribPointer(vertex_shader->vertex_position_attr, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertex_shader->vertex_position_attr);

    Zero(vertex_shader->color_buffer);
    glGenBuffers(1, &vertex_shader->color_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_shader->color_buffer);
    glVertexAttribPointer(vertex_shader->color_attr, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(vertex_shader->color_attr);

    
    
    fragment_shader->texture_uniform = glGetUniformLocation(program, "texture_");
    fragment_shader->texture_present_uniform = glGetUniformLocation(program, "texture_present");

    return (glGetError()) ? false : true;
}

inline
void gpu_frame_init() // TODO @Robustness: Pass in which buffers to clear.
{
    glClear(GL_COLOR_BUFFER_BIT);
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
void gpu_bind_texture(GPU_Texture_ID tex)
{
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
void gpu_set_vertex_buffer_data(GPU_Buffer_ID buffer, void *data, size_t size)
{
    Assert(data != NULL);
    Assert(size >= 0);
    
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

inline
void gpu_draw(GPU_Primitive_Type type, int num_vertices, size_t offset = 0)
{
    Assert(num_vertices >= 0);
    glDrawArrays(type, offset, num_vertices);
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
bool gpu_create_texture(u32 w, u32 h, GPU_Texture_Parameters params, GPU_Texture_ID *_id, GPU_Error_Code *_error_code)
{
    //TODO @Incomplete: Use params
    
    glGenTextures(1, _id);
    
    glBindTexture(GL_TEXTURE_2D, *_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

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
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, params.pixel_components, w, h, 0,
                 params.pixel_format, params.pixel_data_type, data);

    auto gl_error = glGetError();
    Assert(gl_error == 0);
}
