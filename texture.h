
struct Pixel
{
    u8 r, g, b, a;
};

Pixel pixel(u8 r, u8 g, u8 b, u8 a)
{
    Pixel p = {r, g, b, a};
    return p;
}



enum Texture_ID
{
    TEX_FONT_TITLE,
    TEX_FONT_BODY,
    TEX_FONT_INPUT,
    
    TEX_NONE_OR_NUM
};

// @Jai
String texture_name(Texture_ID texture_id)
{
    switch(texture_id)
    {
        case TEX_FONT_TITLE:  return STRING("FONT_TITLE"); break;
        case TEX_FONT_BODY:   return STRING("FONT_BODY");  break;
        case TEX_FONT_INPUT:  return STRING("FONT_INPUT"); break;
            
        case TEX_NONE_OR_NUM:
            Assert(false);
            return EMPTY_STRING;
    }

    //Assert(false);
    return STRING("UNKNOWN TEXTURE");
}

const char *texture_filenames[] = {
    NULL, // Font
    NULL, // Font
    NULL // Font
};
static_assert(ARRLEN(texture_filenames) == TEX_NONE_OR_NUM);

struct Texture_Catalog
{
    GPU_Texture_ID ids[TEX_NONE_OR_NUM];
    v2s sizes[TEX_NONE_OR_NUM];
    bool exists[TEX_NONE_OR_NUM];

    GPU_Texture_Parameters params[TEX_NONE_OR_NUM];

#if DEBUG
    Pixel *pixels[TEX_NONE_OR_NUM];
#endif
};
