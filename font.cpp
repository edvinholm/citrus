


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


inline
Glyph_Table::Bucket &glyph_table_bucket_for_codepoint(int codepoint, Glyph_Table &table)
{
    return table.buckets[codepoint % ARRLEN(table.buckets)];
}




inline float scale_for_font_size(Font_Size fs, Font *font, bool multiply_by_oversampling_rate = false)
{
    return stbtt_ScaleForPixelHeight(&font->stb_info, pixel_height_for_font_size(fs, multiply_by_oversampling_rate));
}

inline
float font_height(Font_Size size, Font *font)
{
    float scale = scale_for_font_size(size, font);
    return (font->ascent - font->descent) * scale;
}

inline
float font_ascent(Font_Size size, Font *font)
{
    return font->ascent * scale_for_font_size(size, font);
}

inline
float font_descent(Font_Size size, Font *font)
{
    return font->descent * scale_for_font_size(size, font);
}

float codepoint_kerning(stbtt_fontinfo *font_info, int cp1, int cp2)
{
    return (float)stbtt_GetCodepointKernAdvance(font_info, cp1, cp2);
}

float glyph_kerning(stbtt_fontinfo *font_info, int glyph_index_1, int glyph_index_2)
{
    return (float)stbtt_GetGlyphKernAdvance(font_info, glyph_index_1, glyph_index_2);
}

bool load_glyph(int codepoint, Font_Size size, Glyph_Info *existing_glyph, Font *font)   
{
    stbtt_fontinfo *font_info = &font->stb_info;
    
    Sprite_Map &map = *font->sprite_map;

    float scale = scale_for_font_size(size, font, true);

    int glyph_index = stbtt_FindGlyphIndex(font_info, codepoint);

    int glyph_w, glyph_h;
    int ix0, iy0, ix1, iy1;
    {
        stbtt_GetGlyphBitmapBox(font_info, glyph_index, scale, scale,
                                    &ix0, &iy0, &ix1, &iy1);
        glyph_w = ix1 - ix0 + 2; // @Hack: Some characters, for example H gets clipped if we don't have this +2...
        glyph_h = iy1 - iy0;
    }

    u32 x, y;
    if(!find_empty_location_in_sprite_map(map, glyph_w, glyph_h, &x, &y))
    {
        Debug_Print("Unable to find a place in the glyph map.\n");
        return false;
    }

    if (glyph_w * glyph_h > 0)
    {
        u8 *mono = (u8 *)tmp_alloc(sizeof(u8) * glyph_w * glyph_h);
        Release_Assert(mono != NULL);

        stbtt_MakeGlyphBitmap(font_info, mono,
            glyph_w, glyph_h, glyph_w,
            scale, scale, glyph_index);

        u8 *dest     = (u8 *)&map.pixels[y * map.w + x];
        u64 dest_jump = 4 * (map.w - glyph_w);
        u8 *dest_end = dest + glyph_w * 4 + (glyph_h-1) * (glyph_w * 4 + dest_jump);
        u8 *src = mono;

        s64 xx = 0;
        s64 glyph_w_minus_8 = glyph_w-8;
        while(dest < dest_end) {

            while(xx < glyph_w_minus_8) {
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = *src++;

                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = *src++;

                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = *src++;

                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = *src++;

                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = *src++;

                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = *src++;

                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = *src++;

                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = *src++;

                xx += 8;
            }

            while(xx < glyph_w) {
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = 0xFF;
                *dest++ = *src++;

                xx++;
            }

             xx = 0;
             dest += dest_jump;
        }
    }
    
    map.needs_update = true;

    

    if(!existing_glyph)
    {
        Glyph_Info glyph = {0};

        Glyph_Table::Bucket &bucket = glyph_table_bucket_for_codepoint(codepoint, font->glyphs);
        array_add(bucket.glyph_codepoints, codepoint);
        array_add(bucket.glyphs, &glyph);

        existing_glyph = last_element_pointer(bucket.glyphs);
    }

    existing_glyph->glyph_index = glyph_index;

    Sized_Glyph &sized = existing_glyph->sized[size];
    sized.loaded = true;
    
    sized.sprite_frame_p0 = V2(x / (float)map.w, y / (float)map.h);
    sized.sprite_frame_p1 = sized.sprite_frame_p0 + V2(glyph_w / (float)map.w, glyph_h / (float)map.h);
    sized.pixel_s = V2U(glyph_w, glyph_h);

    int left_side_bearing;
    int advance_width;
    stbtt_GetGlyphHMetrics(font_info, glyph_index, &advance_width, &left_side_bearing);
    sized.offset = V2(left_side_bearing * scale, iy0) / TWEAK_font_oversampling_rate;
    sized.advance_width = advance_width * scale / TWEAK_font_oversampling_rate;

    return true;
}


//NOTE: Might return null if no glyph was found or loaded
inline
Sized_Glyph *find_or_load_glyph(int codepoint, Font_Size size, Font *font, int *_glyph_index = NULL)
{
    Glyph_Table::Bucket &bucket = glyph_table_bucket_for_codepoint(codepoint, font->glyphs);
    
    Glyph_Info *unsized = NULL;
    Sized_Glyph  *sized = NULL;
    
    for(int i = 0; i < bucket.glyph_codepoints.n; i++)
    {    
        if(bucket.glyph_codepoints[i] != codepoint) continue;

        unsized = bucket.glyphs.e + i;

        Sized_Glyph *sized_ = &unsized->sized[size];
        if(sized_->loaded)
        {
            sized = sized_;
        }
        break;
    }

    if(!sized)
    {
        if(!load_glyph(codepoint, size, unsized, font))
           return NULL;

        if(!unsized)
        {
            unsized = &bucket.glyphs.e[bucket.glyphs.n - 1];
        }
        sized = &unsized->sized[size];
    }
    Assert(unsized);
    Assert(sized);
    Assert(sized->loaded);
    
    if(_glyph_index)
        *_glyph_index = unsized->glyph_index;

    return sized;
}


inline
int glyph_index_for_codepoint(int codepoint, Font *font)
{
    return stbtt_FindGlyphIndex(&font->stb_info, codepoint);
}



//NOTE: Assumes you've already zeroed *_font.
bool load_font(byte *font_file_contents, Font *_font)
{
    if(!stbtt_InitFont(&_font->stb_info, font_file_contents, stbtt_GetFontOffsetForIndex(font_file_contents, 0)))
        return false;

    int ascent_int, descent_int, line_gap_int;
    stbtt_GetFontVMetrics(&_font->stb_info, &ascent_int, &descent_int, &line_gap_int);
    _font->ascent =   ascent_int;
    _font->descent =  descent_int; 
    _font->line_gap = line_gap_int;

    for(int size = 0; size < NUM_FONT_SIZES; size++)
    {
        for(int codepoint = 32; codepoint <= 126; codepoint++)
        {
            if(!load_glyph(codepoint, (Font_Size)size, NULL, _font)) { }
        }
    }
    return true;
}


void init_fonts(Graphics *gfx)
{
    TIMED_FUNCTION;
    
    for(int f = 0; f < NUM_FONTS; f++)
    {
        // IMPORTANT: This does NOT create a gpu texture. The texture will be created in game_init_graphics, so that we can redo it when we lose the graphics context.
        gfx->glyph_maps[f] = create_sprite_map(TWEAK_font_texture_size, TWEAK_font_texture_size, font_textures[f], ALLOC_GFX, gfx);

        Font *font = &gfx->fonts[f];

        font->sprite_map = &gfx->glyph_maps[f];

        char *filename = (char *)font_filenames[f];
        byte *file_contents;
        if(!read_entire_resource(filename, &file_contents, ALLOC_GFX)) // IMPORTANT: Do not dealloc this. stb_truetype needs the data as long as the font lives.
        {
            Debug_Print("Unable to read font resource file '%s'\n", filename);
            continue;
        }
        
        if(!load_font(file_contents, font))
        {
            Debug_Print("Unable to load font from file content ('%s')\n", filename);
            continue;
        }
    }
    
}
