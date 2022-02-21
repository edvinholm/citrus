

enum Font_ID
{
    FONT_TITLE = 0,
    FONT_BODY,
    FONT_INPUT,

    NUM_FONTS
};

Texture_ID font_textures[] = {
    TEX_FONT_TITLE,  // FONT_TITLE
    TEX_FONT_BODY,   // FONT_BODY
    TEX_FONT_INPUT,  // FONT_BODY
};
static_assert(ARRLEN(font_textures) == NUM_FONTS);

const char *font_filenames[] = {
    "RussoOne-Regular.ttf",    // FONT_TITLE
    "Varela-Regular.ttf",      // FONT_BODY
    "VarelaRound-Regular.ttf", // FONT_INPUT
};
static_assert(ARRLEN(font_filenames) == NUM_FONTS);



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
    FS_10 = 0,
    FS_12,
    FS_14,
    FS_16,
    FS_18,
    FS_20,
    FS_24,
    FS_28,
    FS_36,
    FS_48,

    NUM_FONT_SIZES
};


inline
int pixel_height_for_font_size(Font_Size fs, bool multiply_by_oversampling_rate = true)
{
    int h;
    
    switch(fs)
    {
        case FS_10: h = 10; break;
        case FS_12: h = 12; break;
        case FS_14: h = 14; break;
        case FS_16: h = 16; break;
        case FS_18: h = 18; break;
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
        h *= tweak_float(TWEAK_FONT_OVERSAMPLING_RATE);
    return h;
}

Font_Size round_font_size(float pixel_height)
{
    Font_Size result = (Font_Size)0;
    float prev_ph = pixel_height_for_font_size(result, false);
    while(result < NUM_FONT_SIZES-1) {
        auto it = (Font_Size)(result + 1);
        float ph = pixel_height_for_font_size(it, false);
        if(pixel_height <= ph) {
            if(fabs(pixel_height - ph) < fabs(pixel_height - prev_ph)) {
                result = it;
            }
            break;
        }
        result = it;
    }
    return result;
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
        Array<int, ALLOC_MALLOC> glyph_codepoints;
        Array<Glyph_Info, ALLOC_MALLOC> glyphs;
    };

    Bucket buckets[256];
};

struct Glyph_Index_Table
{
    struct Bucket
    {
        Array<int, ALLOC_MALLOC> codepoints;
        Array<int, ALLOC_MALLOC> indices;
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



struct Font_Table
{
    Font fonts[NUM_FONTS];

    template<typename T>
    Font &operator [] (T index)
    {
        Assert(index >= 0);
        Assert(index < ARRLEN(fonts));
               
        return fonts[index];
    }
};
