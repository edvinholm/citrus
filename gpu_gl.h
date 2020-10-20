
typedef GLuint GPU_Buffer_ID;

typedef GLuint GPU_Texture_ID;

typedef GLint GPU_Uniform_ID;
typedef GLint GPU_Attribute_ID;

typedef GLenum GPU_Error_Code;


//FROM: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
enum GPU_Primitive_Type
{
    GPU_TRIANGLES = GL_TRIANGLES,
    GPU_LINES = GL_LINES
};

//FROM: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
enum GPU_Texture_Pixel_Components
{
    //GPU_PIX_COMP_DEPTH_STENCIL   = GL_DEPTH_STENCIL,
    //GPU_PIX_COMP_RG   = GL_RG,
    GPU_PIX_COMP_RGB  = GL_RGB,
    GPU_PIX_COMP_RGBA = GL_RGBA
};


//FROM: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
enum GPU_Texture_Pixel_Format
{
    //GPU_PIX_FORMAT_DEPTH_STENCIL   = GL_DEPTH_STENCIL,
    //GPU_PIX_FORMAT_RG   = GL_RG,
    GPU_PIX_FORMAT_RGB  = GL_RGB,
    GPU_PIX_FORMAT_RGBA = GL_RGBA
};


//FROM: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
enum GPU_Texture_Pixel_Data_Type
{
    GPU_PIX_DATA_UNSIGNED_BYTE = GL_UNSIGNED_BYTE
};


struct GPU_Texture_Parameters
{
    GPU_Texture_Pixel_Components pixel_components;
    GPU_Texture_Pixel_Format     pixel_format;
    GPU_Texture_Pixel_Data_Type  pixel_data_type;
};
