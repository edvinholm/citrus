
/*
#define Ensure_Drawer(Ptr) Body_Text_Drawer default_drawer; \
    if(!Ptr) \
    { \
        default_drawer = body_text_drawer();         \
        Ptr = &default_drawer;                       \
    }
*/


struct Body_Text_Drawer
{
    Font_Size last_size;
    float last_scale;
    
    int previous_codepoint;

    v2 pp;
    
    float ascent;
    float descent;
    float line_height_multiplier;
    
    bool do_wrap;
    bool centered;
    
    v2 p;

    strlength bytes_of_word_left;
    bool found_first_non_whitespace_word_on_line;
};

inline
Body_Text_Drawer body_text_drawer(v2 p, float ascent, float descent,
                                  float line_height_multiplier = 1.0, bool do_wrap = true,
                                  bool centered = false)
{
    Body_Text_Drawer drawer;
    Zero(drawer);

    drawer.last_size = FS_NONE;
    
    drawer.p = p;
    drawer.pp = p + V2_Y * ascent;
    drawer.ascent = ascent;
    drawer.descent = descent;
    drawer.line_height_multiplier = line_height_multiplier;
    drawer.do_wrap = do_wrap;
    drawer.centered = centered;
    return drawer;
}


inline
Font_ID current_font_id(Graphics *gfx)
{
    Assert(gfx->current_font >= 0);
    Assert(gfx->current_font < ARRLEN(gfx->fonts));
    return gfx->current_font;
}

inline
Font *current_font(Graphics *gfx)
{
    return &gfx->fonts[current_font_id(gfx)];
}



inline
Body_Text_Drawer body_text_drawer(Graphics *gfx, v2 p, Font_Size size, bool do_wrap = true, float line_height_multiplier = 1.0, bool centered = false)
{
    Font *font = current_font(gfx);
    return body_text_drawer(p, font_height(size, font), font_descent(size, font), line_height_multiplier, do_wrap, centered);
}

//NOTE: Returns size of rendered glyph. clip_rect does not affect this.
v2 draw_glyph(Sized_Glyph *glyph, v2 p, Texture_ID font_texture, Graphics *gfx,
              Rect *clip_rect = NULL, bool do_draw = true)
{
    Rect a;
    a.p = p;
    a.s = V2(glyph->pixel_s) / TWEAK_font_oversampling_rate;
    
    if(do_draw) {
        
        if(clip_rect) { // @Speed

            Rect clip_a = rect_intersection(a, *clip_rect);

            float clip_x1 = clip_a.x + clip_a.w;
            float clip_y1 = clip_a.y + clip_a.h;

            v2 uv0 = glyph->sprite_frame_p0;
            v2 uv1 = glyph->sprite_frame_p1;

            float uv_dx = (uv1.x - uv0.x);
            float uv_dy = (uv1.y - uv0.y);

            uv0.x += uv_dx * ((clip_a.x - a.x)/a.w);
            uv0.y += uv_dy * ((clip_a.y - a.y)/a.h);

            uv1.x += uv_dx * (clip_x1 - (a.x + a.w))/a.w;
            uv1.y += uv_dy * (clip_y1 - (a.y + a.h))/a.h;
            
            v2 uvs[6];
            quad_uvs(uvs, uv0, uv1);
            draw_rect(clip_a, gfx, uvs, font_texture);
        }
        else {
            v2 uvs[6];
            quad_uvs(uvs, glyph->sprite_frame_p0, glyph->sprite_frame_p1);
            draw_rect_d(p, V2_X * a.w, V2_Y * a.h, gfx, uvs, font_texture);
        }
        
    }

    return a.s;
}



//NOTE: Returns area of rendered glyph
//NOTE: Will change p to the next character's position.
//NOTE: This should be given as much info as possible. The other overload is for doing things implicitly.
inline
Rect draw_codepoint(int codepoint, v2 *p, Font_Size size, float scale, Graphics *gfx,
                    bool offset = false, int previous_codepoint = 0, Rect *clip_rect = NULL,
                    bool do_draw = true)
{
    Font *font = current_font(gfx);
    
    int glyph_index;
    Sized_Glyph *glyph = find_or_load_glyph(codepoint, size, font, &glyph_index);
    
    if(previous_codepoint != 0)
    {
        float scale = scale_for_font_size(size, font);
        p->x += glyph_kerning(&font->stb_info, glyph_index_for_codepoint(previous_codepoint, font), glyph_index) * scale;
    }

    Rect glyph_a;
    glyph_a.p = *p + ((offset) ? glyph->offset : V2_ZERO);
    
    if(glyph) {
        Texture_ID font_texture = gfx->glyph_maps[current_font_id(gfx)].texture;
        glyph_a.s = draw_glyph(glyph, glyph_a.p, font_texture, gfx, clip_rect, do_draw);
    }
    else
        glyph_a.s = V2_ZERO;
    
    p->x += glyph->advance_width;

    return glyph_a;
}


//NOTE: This gets the scale etc for you, if you don't have it.
inline
Rect draw_codepoint(int codepoint, v2 *p, Font_Size size, Graphics *gfx, bool offset = false, int previous_codepoint = 0)
{
    Font *font = current_font(gfx);
    
    return draw_codepoint(codepoint, p, size, scale_for_font_size(size, font), gfx, offset, previous_codepoint);
}



float string_width(String string, Font_Size size, Font *font, int previous_codepoint = 0)
{
    float result = 0;

    float scale = scale_for_font_size(size, font);
    
    byte *at = (byte *)string.data;
    byte *end = (byte *)string.data + string.length;
    while(at < end)
    {
        int codepoint = eat_codepoint(&at);

        int glyph_index;
        Sized_Glyph *glyph = find_or_load_glyph(codepoint, size, font, &glyph_index);
        
        if(glyph)
        {   
            if(previous_codepoint != 0)
                result += glyph_kerning(&font->stb_info, glyph_index_for_codepoint(previous_codepoint, font), glyph_index) * scale;

            result += glyph->advance_width;
        }

        previous_codepoint = codepoint;
    }

    return result;
}


float string_width(String string, Font_Size size, Graphics *gfx, int previous_codepoint = 0)
{
    return string_width(string, size, current_font(gfx), previous_codepoint);
}


inline
v2 string_size(String string, Font_Size size, Font *font)
{
    // @Cleanup: @Speed: We do scale_for_font_size twice, since we do it in string_width as well.
    return V2(string_width(string, size, font), (font->ascent - font->descent) * scale_for_font_size(size, font));
}


inline
v2 string_size(String string, Font_Size size, Font_ID font, Graphics *gfx)
{
    Assert(font >= 0);
    Assert(font <= ARRLEN(gfx->fonts));
    
    return string_size(string, size, &gfx->fonts[font]);
}

inline
v2 string_size(String string, Font_Size size, Graphics *gfx)
{
    return string_size(string, size, current_font_id(gfx), gfx);
}

//NOTE: Returns rect of drawn string
Rect draw_string(String string, v2 p, Font_Size size, Font *font, Font_ID font_id, Graphics *gfx,
                 H_Align h_align = HA_LEFT, V_Align v_align = VA_TOP,
                 int *previous_codepoint = NULL)
{
    Texture_ID font_texture = gfx->glyph_maps[font_id].texture;
    
    int default_previous_codepoint = 0;
    if(!previous_codepoint) previous_codepoint = &default_previous_codepoint;

    if(h_align == HA_LEFT && v_align == VA_TOP)
    {
        // Don't need to calculate string size.
    }
    else
    {
        // Alignment
        
        v2 str_size = string_size(string, size, font);
        
        if(h_align == HA_CENTER)
            p.x -= str_size.w / 2.0;
        else if(h_align == HA_RIGHT)
            p.x -= str_size.w;
        else Assert(h_align == HA_LEFT);

        if(v_align == VA_CENTER)
            p.y -= str_size.h / 2.0;
        else if(v_align == VA_BOTTOM)
            p.y -= str_size.h;
        else Assert(v_align == VA_TOP);
    }
    
    float scale = scale_for_font_size(size, font);
    
    v2 pp = p + V2_Y * font->ascent * scale;

    float max_x = pp.x;
    
    byte *at = (byte *)string.data;
    byte *end = (byte *)string.data + string.length;
    while(at < end)
    {
        int codepoint = eat_codepoint(&at);

        if(codepoint == '\n')
        {
            pp.x = p.x;
            pp.y += font_height(size, font);
        }
        else
        {
            int glyph_index;
            Sized_Glyph *glyph = find_or_load_glyph(codepoint, size, font, &glyph_index);

            if(glyph)
            {
                if(previous_codepoint != 0)
                    pp.x += glyph_kerning(&font->stb_info, glyph_index_for_codepoint(*previous_codepoint, font), glyph_index) * scale;

                draw_glyph(glyph, pp + glyph->offset, font_texture, gfx);

                pp.x += glyph->advance_width;
                if(pp.x > max_x)
                    max_x = pp.x;
            }
        }

        *previous_codepoint = codepoint;
    }

    return rect(p, V2(max_x - p.x, pp.y - p.y - font->descent * scale));
}

inline
Rect draw_string(String string, v2 p, Font_Size size, Graphics *gfx,
                 H_Align h_align = HA_LEFT, V_Align v_align = VA_TOP,
                 int *previous_codepoint = NULL)
{
    return draw_string(string, p, size, current_font(gfx), current_font_id(gfx), gfx, h_align, v_align, previous_codepoint);
}


#if 0 // @Cleanup: These should be part of the UI build now when we separated UI build and UI draw.
inline
Rect text(Graphics *gfx, String string, Font_Size size,
          H_Align h_align = HA_LEFT, V_Align v_align = VA_TOP,
          int *previous_codepoint = NULL)
{
    // @Boilerplate
    Rect a = area(gfx);
    v2 p = a.p;
    
    if(h_align == HA_CENTER)
        p.x += a.w / 2.0f;
    else if(h_align == HA_RIGHT)
        p.x += a.w;

    if(v_align == VA_CENTER)
        p.y += a.h / 2.0f;
    else if(v_align == VA_BOTTOM)
        p.y += a.h;
    //----
 
    return draw_string(string, p, size, h_align, v_align, previous_codepoint);
}

inline
Rect text_colored(String string, Font_Size size, v4 color,
                  H_Align h_align = HA_LEFT, V_Align v_align = VA_TOP,
                  int *previous_codepoint = NULL)
{
    _COLOR_(color);
    return draw_string(string, area().p, size, h_align, v_align, previous_codepoint);
}

#endif



inline
Rect draw_string(String string, v2 p, Font_Size size, Font_ID font_id, Graphics *gfx,
                 H_Align h_align = HA_LEFT, V_Align v_align = VA_TOP,
                 int *previous_codepoint = NULL)
{
    Assert(font_id >= 0);
    Assert(font_id < ARRLEN(gfx->fonts));
    
    Font *font = &gfx->fonts[font_id];
    return draw_string(string, p, size, font, font_id, gfx, h_align, v_align, previous_codepoint);
}

#if 0 // @Cleanup: This should be part of the UI build now when we separated UI build and UI draw.
inline
Rect text(String string, Font_Size size, Font_ID font,
          H_Align h_align = HA_LEFT, V_Align v_align = VA_TOP,
          int *previous_codepoint = NULL)
{
    // @Boilerplate
    Rect a = area();
    v2 p = a.p;
    
    if(h_align == HA_CENTER)
        p.x += a.w / 2.0f;
    else if(h_align == HA_RIGHT)
        p.x += a.w;
    
    if(v_align == VA_CENTER)
        p.y += a.h / 2.0f;
    else if(v_align == VA_BOTTOM)
        p.y += a.h;
    //---
        
    return draw_string(string, p, size, font, h_align, v_align, previous_codepoint);
}

#endif



inline
void draw_string_in_rect_centered(String string, Rect a, Font_Size size, Font_ID font, Graphics *gfx)
{
    v2 string_s = string_size(string, size, font, gfx);
    v2 p = a.p + a.s/2.0f - string_s/2.0f;
    draw_string(string, p, size, font, gfx);
}

void draw_string_in_rect_centered(String string, Rect a, Font_Size size, Graphics *gfx)
{
    draw_string_in_rect_centered(string, a, size, current_font_id(gfx), gfx);
}

