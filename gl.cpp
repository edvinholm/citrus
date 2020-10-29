

#if OS_WINDOWS || OS_ANDROID



#if !(OS_ANDROID)
typedef void (*gl_depth_range_f)(GLfloat nearVal, GLfloat farVal);
gl_depth_range_f glDepthRangef;

typedef void (*gl_bind_buffer)(GLenum target, GLuint buffer);
gl_bind_buffer glBindBuffer;

typedef void (*gl_buffer_data)(GLenum target, GLsizei size, const GLvoid * data, GLenum usage);
gl_buffer_data glBufferData;

typedef void (*gl_buffer_sub_data)(GLenum target, GLint offset, GLsizei size, const GLvoid * data);
gl_buffer_sub_data glBufferSubData;

typedef void (*gl_active_texture)(GLenum texture);
gl_active_texture glActiveTexture;

typedef void (*gl_gen_buffers)(GLsizei n, GLuint *buffers);
gl_gen_buffers glGenBuffers;
#endif





typedef GLuint (*gl_create_shader)(GLenum shaderType);
gl_create_shader glCreateShader;

typedef void (*gl_shader_source)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
gl_shader_source glShaderSource;

typedef void (*gl_compile_shader)(GLuint shader);
gl_compile_shader glCompileShader;

typedef GLuint (*gl_create_program)();
gl_create_program glCreateProgram;

typedef GLuint (*gl_attach_shader)(GLuint program, GLuint shader);
gl_attach_shader glAttachShader;

typedef void (*gl_link_program)(GLuint program);
gl_link_program glLinkProgram;

typedef void (*gl_validate_program)(GLuint program);
gl_validate_program glValidateProgram;

typedef void (*gl_get_program_iv)(GLuint program, GLenum pname, GLint *params);
gl_get_program_iv glGetProgramiv;

typedef void (*gl_get_program_info_log)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
gl_get_program_info_log glGetProgramInfoLog;

typedef void (*gl_get_shader_info_log)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
gl_get_shader_info_log glGetShaderInfoLog;

typedef GLint (*gl_get_uniform_location)(GLuint program, const GLchar *name);
gl_get_uniform_location glGetUniformLocation;

typedef GLint (*gl_get_attrib_location)(GLuint program, const GLchar *name);
gl_get_uniform_location glGetAttribLocation;

typedef void (*gl_uniform_matrix_4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
gl_uniform_matrix_4fv glUniformMatrix4fv;

typedef void (*gl_use_program)(GLuint program);
gl_use_program glUseProgram;

typedef void (*gl_vertex_attrib_pointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer);
gl_vertex_attrib_pointer glVertexAttribPointer;

typedef void (*gl_vertex_attrib_i_pointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer);
gl_vertex_attrib_i_pointer glVertexAttribIPointer;

typedef void (*gl_enable_vertex_attrib_array)(GLuint index);
gl_enable_vertex_attrib_array glEnableVertexAttribArray;

typedef void (*gl_uniform_1i)(GLint location, GLint v0);
gl_uniform_1i glUniform1i;

typedef void (*gl_uniform_4fv)(GLint location, GLsizei count, const GLfloat *value);
gl_uniform_4fv glUniform4fv;

typedef void (*gl_uniform_3fv)(GLint location, GLsizei count, const GLfloat *value);
gl_uniform_3fv glUniform3fv;

typedef void (*gl_get_texture_level_parameter_iv)(GLuint texture, GLint level, GLenum pname, GLint *params);
gl_get_texture_level_parameter_iv glGetTextureLevelParameteriv;

typedef void (*gl_get_texture_image)(GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
gl_get_texture_image glGetTextureImage;

typedef void (*gl_gen_framebuffers)(GLsizei n, GLuint *ids);
gl_gen_framebuffers glGenFramebuffers;

typedef void (*gl_bind_framebuffer)(GLenum target, GLuint framebuffer);
gl_bind_framebuffer glBindFramebuffer;

typedef GLenum (*gl_check_framebuffer_status)(GLenum target);
gl_check_framebuffer_status glCheckFramebufferStatus;

typedef void (*gl_draw_buffers)(GLsizei n, const GLenum *bufs);
gl_draw_buffers glDrawBuffers;

typedef void (*gl_framebuffer_texture)(GLenum target, GLenum attachment, GLuint texture, GLint level);
gl_framebuffer_texture glFramebufferTexture;

typedef void (*gl_gen_renderbuffers)(GLsizei n, GLuint *renderbuffers);
gl_gen_renderbuffers glGenRenderbuffers;

typedef void (*gl_bind_renderbuffer)(GLenum target, GLuint renderbuffer);
gl_bind_renderbuffer glBindRenderbuffer;

typedef void (*gl_renderbuffer_storage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
gl_renderbuffer_storage glRenderbufferStorage;

typedef void (*gl_named_renderbuffer_storage)(GLuint renderbuffer, GLsizei width, GLsizei height);
gl_named_renderbuffer_storage glNamedRenderbufferStorage;

typedef void (*gl_framebuffer_renderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
gl_framebuffer_renderbuffer glFramebufferRenderbuffer;

typedef void (*gl_delete_renderbuffers)(GLsizei n, GLuint *renderbuffers);
gl_delete_renderbuffers glDeleteRenderbuffers;

typedef void (*gl_tex_image_2d_multisample)(GLenum target, GLsizei samples, GLint internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
gl_tex_image_2d_multisample glTexImage2DMultisample;

typedef void (*gl_framebuffer_texture_2d)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
gl_framebuffer_texture_2d glFramebufferTexture2D;

typedef void (*gl_blit_framebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
gl_blit_framebuffer glBlitFramebuffer;

#endif


#define CHECK_GL_EXT_LOADED(Proc) if(!Proc) { Debug_Print("Error loading GL Extension %s at %s:%d\n", #Proc, __FILE__, __LINE__); Result = false; }

#if OS_WINDOWS
#define TRY_LOAD_GL_EXT(Type, Name) Name = (Type)wglGetProcAddress(#Name); \
    CHECK_GL_EXT_LOADED(Name)
#elif OS_ANDROID
#define TRY_LOAD_GL_EXT(Type, Name) Name = (Type)eglGetProcAddress(#Name); \
    CHECK_GL_EXT_LOADED(Name)
#endif

bool load_gl_extensions()
{
        //TODO @Robustness: We should check if we actually use these functions.
    bool Result = true;
    
#if OS_WINDOWS || OS_ANDROID


#if !(OS_ANDROID)
    TRY_LOAD_GL_EXT(gl_depth_range_f, glDepthRangef);
    TRY_LOAD_GL_EXT(gl_bind_buffer, glBindBuffer);
    TRY_LOAD_GL_EXT(gl_buffer_data, glBufferData);
    TRY_LOAD_GL_EXT(gl_buffer_sub_data, glBufferSubData);
    TRY_LOAD_GL_EXT(gl_active_texture, glActiveTexture);
    TRY_LOAD_GL_EXT(gl_gen_buffers, glGenBuffers);
#endif

    
    TRY_LOAD_GL_EXT(gl_create_shader, glCreateShader);
    TRY_LOAD_GL_EXT(gl_shader_source, glShaderSource);
    TRY_LOAD_GL_EXT(gl_compile_shader, glCompileShader);
    TRY_LOAD_GL_EXT(gl_create_program, glCreateProgram);
    TRY_LOAD_GL_EXT(gl_attach_shader, glAttachShader);
    TRY_LOAD_GL_EXT(gl_link_program, glLinkProgram);
    TRY_LOAD_GL_EXT(gl_validate_program, glValidateProgram);
    TRY_LOAD_GL_EXT(gl_get_program_iv, glGetProgramiv);
    TRY_LOAD_GL_EXT(gl_get_program_info_log, glGetProgramInfoLog);
    TRY_LOAD_GL_EXT(gl_get_shader_info_log, glGetShaderInfoLog);
    TRY_LOAD_GL_EXT(gl_get_uniform_location, glGetUniformLocation);
    TRY_LOAD_GL_EXT(gl_get_attrib_location, glGetAttribLocation);
    TRY_LOAD_GL_EXT(gl_uniform_matrix_4fv, glUniformMatrix4fv);
    TRY_LOAD_GL_EXT(gl_use_program, glUseProgram);
    TRY_LOAD_GL_EXT(gl_vertex_attrib_pointer, glVertexAttribPointer);
    TRY_LOAD_GL_EXT(gl_vertex_attrib_i_pointer, glVertexAttribIPointer);
    TRY_LOAD_GL_EXT(gl_enable_vertex_attrib_array, glEnableVertexAttribArray);
    TRY_LOAD_GL_EXT(gl_uniform_1i, glUniform1i);
    TRY_LOAD_GL_EXT(gl_uniform_4fv, glUniform4fv);
    TRY_LOAD_GL_EXT(gl_uniform_3fv, glUniform3fv);
    TRY_LOAD_GL_EXT(gl_gen_framebuffers, glGenFramebuffers);
    TRY_LOAD_GL_EXT(gl_bind_framebuffer, glBindFramebuffer);
    TRY_LOAD_GL_EXT(gl_check_framebuffer_status, glCheckFramebufferStatus);
    TRY_LOAD_GL_EXT(gl_draw_buffers, glDrawBuffers);
    TRY_LOAD_GL_EXT(gl_framebuffer_texture, glFramebufferTexture);
    TRY_LOAD_GL_EXT(gl_gen_renderbuffers, glGenRenderbuffers);
    TRY_LOAD_GL_EXT(gl_bind_renderbuffer, glBindRenderbuffer);
    TRY_LOAD_GL_EXT(gl_renderbuffer_storage, glRenderbufferStorage);
    TRY_LOAD_GL_EXT(gl_framebuffer_renderbuffer, glFramebufferRenderbuffer);
    TRY_LOAD_GL_EXT(gl_delete_renderbuffers, glDeleteRenderbuffers);
    TRY_LOAD_GL_EXT(gl_tex_image_2d_multisample, glTexImage2DMultisample);
    TRY_LOAD_GL_EXT(gl_framebuffer_texture_2d, glFramebufferTexture2D);
    TRY_LOAD_GL_EXT(gl_blit_framebuffer, glBlitFramebuffer);

#endif
    
    return Result;
}



GLuint create_gl_program(u8 *VertexShader, u8 *FragmentShader, GLint *_ProgramID)
{
    GLint ShaderCodeLengths[] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLchar *VertexShaderCode[] = {
        (GLchar *)VertexShader
    };
    glShaderSource(VertexShaderID, 1, (const GLchar **)VertexShaderCode, ShaderCodeLengths);
    glCompileShader(VertexShaderID);
    
    
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    GLchar *FragmentShaderCode[] = {
        (GLchar *)FragmentShader
    };
    glShaderSource(FragmentShaderID, 1, (const GLchar **)FragmentShaderCode, ShaderCodeLengths);
    glCompileShader(FragmentShaderID);
    
    
    GLint GLProgramID = glCreateProgram();
    glAttachShader(GLProgramID, VertexShaderID);
    glAttachShader(GLProgramID, FragmentShaderID);
    glLinkProgram(GLProgramID);
    
    GLint Linked = false;
    //    GLint Compiled = false;
    glValidateProgram(GLProgramID);
    glGetProgramiv(GLProgramID, GL_LINK_STATUS, &Linked);
    //    glGetProgramiv(GLProgramID, GL_COMPILE_STATUS, &Compiled);
    
    if(!Linked)
    {
        GLsizei Ignored;
        
        char VertexErrors[4096];
        char FragmentErrors[4096];
        char ProgramErrors[4096];
        
        glGetShaderInfoLog(VertexShaderID, sizeof(VertexErrors), &Ignored, VertexErrors);
        glGetShaderInfoLog(FragmentShaderID, sizeof(FragmentErrors), &Ignored, FragmentErrors);
        glGetProgramInfoLog(GLProgramID, sizeof(ProgramErrors), &Ignored, ProgramErrors);
        
        Debug_Print("\nSHADER PROGRAM VALIDATION FAILED\n");
        Debug_Print("----------------------------------\n");

        Debug_Print("Program:\n%s\n", ProgramErrors);
        Debug_Print("Vertex shader:\n%s\n", VertexErrors);
        Debug_Print("Fragment shader:\n%s\n", FragmentErrors);

        Debug_Print("----------------------------------\n\n");

    }
    
    *_ProgramID = GLProgramID;
    return Linked;
}


