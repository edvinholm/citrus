
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

    TEX_PREVIEWS,
    
    TEX_NONE_OR_NUM
};

String texture_names[] = {
    STRING("FONT_TITLE"),
    STRING("FONT_BODY"),
    STRING("FONT_INPUT"),
    
    STRING("PREVIEWS"),
};
static_assert(ARRLEN(texture_names) == TEX_NONE_OR_NUM);

const char *texture_filenames[] = {
    NULL, // Font
    NULL, // Font
    NULL, // Font

    NULL // Previews
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
