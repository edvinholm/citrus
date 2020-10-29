

enum Font_ID
{
    FONT_TITLE = 0,

    NUM_FONTS
};

Texture_ID font_textures[NUM_FONTS] = {
    TEX_FONT_TITLE, // FONT_TITLE
};

const char *font_filenames[NUM_FONTS] = {
    "RussoOne-Regular.ttf", // FONT_TITLE
};



enum H_Align
{
    HA_LEFT,
    HA_CENTER,
    HA_RIGHT
};

enum V_Align
{
    VA_TOP,
    VA_CENTER,
    VA_BOTTOM
};




enum Font_Size
{
    FS_NONE = -1,
    FS_10,
    FS_12,
    FS_14,
    FS_16,
    FS_20,
    FS_24,
    FS_28,
    FS_36,
    FS_48,

    NUM_FONT_SIZES
};


struct Sized_Glyph //@Cleanup: @BadName
{
    bool loaded;

    v2 sprite_frame_p0; //In sprite map, 0.0 - 1.0
    v2 sprite_frame_p1; //In sprite map, 0.0 - 1.0
    v2u pixel_s;

    //TODO @Cleanup: @Memory: unbake scale and put these in Glyph_Info.
    float advance_width; // Scale is baked in
    v2 offset;           // Scale is baked in. This is the offset from the glyph's origin.
    //--------------
};

struct Glyph_Info
{
    Sized_Glyph sized[NUM_FONT_SIZES];
    int glyph_index;
};


struct Glyph_Table
{
    struct Bucket
    {
        Array<int, ALLOC_GFX> glyph_codepoints;
        Array<Glyph_Info, ALLOC_GFX> glyphs;
    };

    Bucket buckets[256];
};

struct Font
{
    Sprite_Map *sprite_map;
    
    Glyph_Table glyphs;

    float ascent;   // Scale SHOULD NOT BE baked in
    float descent;  // Scale SHOULD NOT BE baked in
    float line_gap; // Scale SHOULD NOT BE baked in

    //@Temporary ?
    stbtt_fontinfo stb_info;
};

