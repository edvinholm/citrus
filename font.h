

enum Font_ID
{
    FONT_TITLE = 0,
    FONT_BODY,
    FONT_INPUT,

    NUM_FONTS
};

Texture_ID font_textures[NUM_FONTS] = {
    TEX_FONT_TITLE,  // FONT_TITLE
    TEX_FONT_BODY,   // FONT_BODY
    TEX_FONT_INPUT,  // FONT_BODY
};

const char *font_filenames[NUM_FONTS] = {
    "RussoOne-Regular.ttf",    // FONT_TITLE
    "Varela-Regular.ttf",      // FONT_BODY
    "VarelaRound-Regular.ttf", // FONT_INPUT
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


inline int pixel_height_for_font_size(Font_Size fs, bool multiply_by_oversampling_rate = true)
{
    int h;
    
    switch(fs)
    {
        case FS_10: h = 10; break;
        case FS_12: h = 12; break;
        case FS_14: h = 14; break;
        case FS_16: h = 16; break;
        case FS_20: h = 20; break;
        case FS_24: h = 24; break;
        case FS_28: h = 28; break;
        case FS_36: h = 36; break;
        case FS_48: h = 48; break;

        case NUM_FONT_SIZES:
        default:
            Assert(false);
            return 0;
    }

    if(multiply_by_oversampling_rate)
        h *= TWEAK_font_oversampling_rate;
    return h;
}


struct Sized_Glyph //@Cleanup: @BadName
{
    bool loaded;

    v2 sprite_frame_p0; //In sprite map, 0.0 - 1.0
    v2 sprite_frame_p1; //In sprite map, 0.0 - 1.0
    v2 pixel_s;

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

struct Glyph_Index_Table
{
    struct Bucket
    {
        Array<int, ALLOC_GFX> codepoints;
        Array<int, ALLOC_GFX> indices;
    };

    Bucket buckets[256];
};

struct Font
{
    Sprite_Map *sprite_map;
    
    Glyph_Table glyphs;

    // NOTE: This are our cache of mappings from codepoints to glyph indices
    //       If we don't find our index here, we ask stbtt.
    Glyph_Index_Table glyph_indices;
    // ///////////////

    float ascent;   // Scale SHOULD NOT BE baked in
    float descent;  // Scale SHOULD NOT BE baked in
    float line_gap; // Scale SHOULD NOT BE baked in

    //@Temporary ?
    stbtt_fontinfo stb_info;
};
