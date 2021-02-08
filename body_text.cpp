

float body_text_line_width(int line_index, Body_Text *bt, Graphics *gfx);

Rect glyph_area(Sized_Glyph *glyph, int glyph_index, v2 p, int last_glyph_index, Font *font,
                float scale, v2 *_new_p, int previous_codepoint = 0)
{
    p.x += glyph_kerning(&font->stb_info, last_glyph_index, glyph_index) * scale;

    Rect glyph_a;

    glyph_a.x = p.x + glyph->offset.x;
    glyph_a.y = p.y + glyph->offset.y + font_ascent(scale, font);
    glyph_a.s = { glyph->pixel_s.w, glyph->pixel_s.h };
    
    p.x += glyph->advance_width;

    *_new_p = p;
    return glyph_a;
}

// IMPORTANT: *_glyph can be set to NULL
Rect codepoint_area(int cp, v2 p, Font_Size size, Font *font, float scale, v2 *_new_p,
                    int previous_codepoint = 0, Sized_Glyph **_glyph = NULL)
{
    int glyph_index;
    Sized_Glyph *glyph = find_or_load_glyph(cp, size, font, &glyph_index);

    int last_glyph_index = -1;
    if(previous_codepoint != 0)
        last_glyph_index = glyph_index_for_codepoint(previous_codepoint, font);

    Rect glyph_a;
    
    if(_glyph) *_glyph = glyph;

    if(glyph)
    {
        return glyph_area(glyph, glyph_index, p, last_glyph_index, font, scale, _new_p);
    }
    else
    {
        glyph_a = {p.x, p.y, 0, 0};
        *_new_p = p;
    }

    return glyph_a;
}

float codepoint_x1(int cp, v2 p, Font_Size size, Font *font, float scale, v2 *_new_p, int previous_codepoint = 0)
{
    int glyph_index;
    Sized_Glyph *glyph = find_or_load_glyph(cp, size, font, &glyph_index);

    if(!glyph) {
        *_new_p = p;
        return p.x;
    }
    
    int last_glyph_index = glyph_index_for_codepoint(previous_codepoint, font);

    p.x += glyph_kerning(&font->stb_info, last_glyph_index, glyph_index) * scale;

    float x1 = p.x + glyph->offset.x + glyph->pixel_s.w;

    *_new_p = { p.x + glyph->advance_width, p.y };

    return x1;
}


// Get position in the text where a line starts and ends, in a Body_Text
void get_line_start_and_end(int line_index, Body_Text *bt, u8 **_start, u8 **_end)
{
    Assert(line_index >= 0);
    Assert(line_index < bt->lines.n);

    // Get our line
    Body_Text_Line line = bt->lines[line_index];
    
    // Start is easy.
    *_start = bt->text.data + line.start_byte;

    if(line_index < bt->lines.n - 1) {
        u8 *next_line_start =  bt->text.data + bt->lines[line_index + 1].start_byte;

        u8 *start_of_last_cp_on_this_line = find_codepoint_backwards(next_line_start);
        u32 last_cp_on_this_line = eat_codepoint(&start_of_last_cp_on_this_line);
        if(last_cp_on_this_line == '\n') //TODO @Robustness: Do is_newline() here (Add to ucd.gen)
            *_end = start_of_last_cp_on_this_line;
        else
            *_end = next_line_start;
    }
    else
        *_end = bt->text.data + bt->text.length;  // End of text
}

// Get position in the text where a line starts and ends, in a Body_Text
void get_line_cp_start_and_end(int line_index, Body_Text *bt, int *_start_cp_index, int *_end_cp_index)
{
    Assert(line_index >= 0);
    Assert(line_index < bt->lines.n);

    // Get our line
    Body_Text_Line line = bt->lines[line_index];
    
    // Start is easy.
    *_start_cp_index = line.start_cp;

    // End is start cp of next line or total number of codepoints in text if our line is the last one.
    if(line_index < bt->lines.n - 1)
        *_end_cp_index = bt->lines[line_index + 1].start_cp; // Start of next line
    else
        *_end_cp_index = bt->num_codepoints;  // End of text
}


// NOTE: If place_before_trailing_whitespace is true, we will get the index of the last codepoint before end, if the last codepoint is a whitespace. Instead of last codepoint index + 1.
int codepoint_index_from_x(float x, u8 *start, u8 *end, Body_Text *bt, Font *fonts, bool place_before_trailing_whitespace, strlength *_rel_byte/* = NULL*/)
{
    v2 pp = V2_ZERO;

    u8 *at = start;
    int prev_cp = 0;
    u8 *prev_cp_start = start; // Not really, but whatevs. It works.

    Font *font = &fonts[bt->font];

    int cp_index = 0;
    while (at < end)
    {
        // Get codepoint
        u8 *cp_start = at;
        int cp = eat_codepoint(&at);

        // Get glyph and area
        float p_x0 = pp.x;
        codepoint_x1(cp, pp, bt->font_size, font, bt->glyph_scale, &pp, prev_cp);

        // Calculate center x of glyph
        float glyph_mid_x = (p_x0 + pp.x) / 2.0f;

        // If the x we're searching for is to the left of this glyph's center, this is the codepoint we want to return.
        if (glyph_mid_x > x) {
            if (_rel_byte) *_rel_byte = (cp_start - start);
            return cp_index;
        }

        // Set previous codepoint and current codepoint index
        prev_cp       = cp;
        prev_cp_start = cp_start;
        cp_index++;
    }

    strlength rel_byte = (strlength)(end - start);

    if (place_before_trailing_whitespace && is_whitespace(prev_cp)) {
        cp_index--;
        rel_byte = (strlength)(prev_cp_start - start);

        Assert(cp_index >= 0);
    }

    if(_rel_byte) *_rel_byte = rel_byte;
    return cp_index;
}

float x_from_codepoint_index(int cp_index_to_find, u8 *start, u8 *end, Body_Text *bt, Font *fonts, int *_cp/* = NULL*/)
{
    v2 pp = V2_ZERO;

    Font *font = &fonts[bt->font];
    
    u8 *at = start;
    int prev_cp = 0;
    int cp_index = 0;
    while(at < end)
    {
        // Get codepoint
        int cp = eat_codepoint(&at);

        if (cp_index == cp_index_to_find)
        {
            if (_cp) *_cp = cp;
            return pp.x;
        }
                        
        // Get glyph and area
        Rect glyph_a = codepoint_area(cp, pp, bt->font_size, font, bt->glyph_scale, &pp, prev_cp);

        // Set previous codepoint
        prev_cp = cp;

        cp_index++;
    }

    Assert(cp_index == cp_index_to_find);
    
    if (_cp) *_cp = 0;
    return pp.x;
}

float y_from_line_index(int line_index, Body_Text *bt)
{
    Assert(line_index >= 0);
    Assert(line_index < bt->lines.n);

    return line_index * bt->line_height;
}

Text_Location text_location_from_position(v2 p, Body_Text *bt, v2 bt_p, Font *fonts)
{
    v2 delta = p - bt_p;
    s64 line_index = delta.y / bt->line_height;

    // Clamp line to valid interval
    line_index = clamp(line_index, (s64)0, bt->lines.n - 1);

    // Get our line
    Body_Text_Line line = bt->lines[line_index];

    // Get line start and end position in the text //
    u8 *line_start;
    u8 *line_end;
    get_line_start_and_end(line_index, bt, &line_start, &line_end);
    // -- 

    // Create location struct and assign line index
    Text_Location loc;
    loc.line = line_index;

    // Find col and byte index in our line.
    strlength byte_in_line;
    loc.col = codepoint_index_from_x(delta.x, line_start, line_end, bt, fonts, true, &byte_in_line);

    // Calculate "global" codepoint and byte index in text
    loc.cp_index = line.start_cp + loc.col;
    loc.byte = line.start_byte + byte_in_line;

    return loc;
}

int line_from_codepoint_index(int cp_index, Body_Text *bt, int *_cp_index_on_line/* = NULL*/)
{
    Assert(cp_index >= 0);
    Assert(cp_index <= bt->num_codepoints);
    Assert(bt->lines.n > 0);

    if (cp_index == bt->num_codepoints) {
        auto &line = bt->lines[bt->lines.n-1];
        if(_cp_index_on_line) *_cp_index_on_line = bt->num_codepoints - line.start_cp;
        return bt->lines.n-1;
    }
    
    // Binary search lines for the cp_index //
    int low_line = 0;
    int high_line = bt->lines.n - 1;

    // Set this to the line we found.
    int found_line = -1;
    
    while(low_line <= high_line)
    {
        int mid_line = (low_line + high_line) / 2;

        // Get start and end codepoint of the currently tested line 
        int mid_start_cp, mid_end_cp;
        get_line_cp_start_and_end(mid_line, bt, &mid_start_cp, &mid_end_cp);

        if(cp_index < mid_start_cp)
        {
            // Search left
            high_line = mid_line - 1;
            continue;
        }

        if(cp_index > mid_end_cp ||
           (mid_line != bt->lines.n-1 && cp_index == mid_end_cp)) // cp index after last cp counts as being part of last line.
        {
            // Search right
            low_line = mid_line + 1;
            continue;
        }

        found_line = mid_line;
        if(_cp_index_on_line) *_cp_index_on_line = cp_index - mid_start_cp;
        break;
    }

    // We should find a line with correct input.
    // Incorrect input should already have caused an assert to fail.
    Assert(found_line != -1);

    return found_line;
}


v2 position_from_codepoint_index(int cp_index, Body_Text *bt, Font *fonts, int *_cp = NULL, int *_line_index = NULL)
{
    Assert(cp_index >= 0);
    Assert(cp_index <= bt->num_codepoints);
    
    // Find which line this codepoint is on //
    int line_index = line_from_codepoint_index(cp_index, bt);
    int rel_cp_index = cp_index - bt->lines[line_index].start_cp;

    // Find x in the found line //
    u8 *line_start;
    u8 *line_end;
    get_line_start_and_end(line_index, bt, &line_start, &line_end);

    float x = x_from_codepoint_index(rel_cp_index, line_start, line_end, bt, fonts, _cp);
    // ---- //

    if(_line_index) *_line_index = line_index;
    
    return { x, y_from_line_index(line_index, bt) };
}

Rect area_from_codepoint_index(int cp_index, Body_Text *bt, Font *fonts, int *_cp = NULL)
{
    Rect a;
    int cp = 0;
    a.p = position_from_codepoint_index(cp_index, bt, fonts, &cp);

    if(_cp) *_cp = cp;
    v2 unused;
    return codepoint_area(cp, a.p, bt->font_size, &fonts[bt->font], bt->glyph_scale, &unused);
}

void draw_body_text(Body_Text *bt, v2 p, v4 color, Graphics *gfx, H_Align h_align = HA_LEFT, Rect *clip_rect = NULL)
{
    Font *font = &gfx->fonts[bt->font];
    Texture_ID font_texture = gfx->glyph_maps[bt->font].texture;

    float texture_slot = bound_slot_for_texture(font_texture, gfx);
    
    float glyph_vertex_textures[6] = {
        texture_slot, texture_slot, texture_slot,
        texture_slot, texture_slot, texture_slot
    };
    
    v4 glyph_vertex_colors[6] = {
        color, color, color,
        color, color, color
    };

    
    u8 *end = bt->text.data + bt->text.length;
    
    v2 pp = p;
    pp.x += bt->start_x;


    // DRAW LINES //
    for(int l = 0; l < bt->lines.n; l++)
    {
        if(h_align == HA_CENTER) {            
            pp.x += bt->w / 2.0f - body_text_line_width(l, bt, gfx) / 2.0f;
        } else if(h_align == HA_RIGHT) {
            pp.x += bt->w - body_text_line_width(l, bt, gfx);
        }
        
        // Find line start and end for line_index = l //
        u8 *line_start;
        u8 *line_end;
        get_line_start_and_end(l, bt, &line_start, &line_end);
        // -- //

        float line_x0 = pp.x;

        bool do_draw_line = true;
        Rect *line_clip_rect = NULL;
        if(clip_rect) {
            if(pp.y + bt->line_height < clip_rect->y) {
                do_draw_line = false; // Past top border of clip rect.
            }
            else if(pp.y >= clip_rect->y + clip_rect->h) {
                break; // Past bottom border of clip rect.
            }
            else if(pp.y < clip_rect->y || pp.y + bt->line_height > clip_rect->y + clip_rect->h)
               line_clip_rect = clip_rect;
        }

        // Draw glyphs //
        u8 *at = line_start;
        int prev_cp = 0;
        int cp_index = 0;
        while(at < line_end)
        {
            // Get codepoint
            int cp = eat_codepoint(&at);
            if(cp != '\n' && cp != '\r')
            {
                Rect *glyph_clip_rect = line_clip_rect;
                
                // Get glyph and area
                Sized_Glyph *glyph;
                Rect glyph_a = codepoint_area(cp, pp, bt->font_size, font, bt->glyph_scale, &pp, prev_cp, &glyph);
                if (cp_index == 0)
                {
                    float delta = line_x0 - glyph_a.x;
                    glyph_a.x += delta;
                    pp.x += delta;
                }

                // Clip?
                if(clip_rect && !glyph_clip_rect) {
                    if(glyph_a.x < clip_rect->x || glyph_a.x + glyph_a.w > clip_rect->x + clip_rect->w)
                        glyph_clip_rect = clip_rect;
                }
                
                // Draw glyph in that area.
                if(glyph) draw_glyph(glyph, glyph_a.p, glyph_vertex_colors, glyph_vertex_textures, gfx, glyph_clip_rect, do_draw_line);

                cp_index++;
            }
            
            // Set previous codepoint
            prev_cp = cp;
        }

        // Prepare position for next line
        pp.x = p.x;
        pp.y += bt->line_height;
    }
}

// TODO @Cleanup: Take a size instead of a rect
Body_Text create_body_text(String text, Rect a, Font_Size font_size, Font_ID font_id, Font *fonts, float start_x/* = 0*/, bool multiline/* = true*/)
{
    float a_x1 = a.x + a.w;

    Font *font = &fonts[font_id];
    
    // SETUP THINGS WE ALREADY KNOW ABOUT OUR Body_Text //
    Body_Text bt = {0};
    bt.text = text;

    bt.start_x = start_x;
    bt.w = a.w;
    
    bt.font = font_id;
    bt.font_size = font_size;

    bt.ascent      = font_ascent(font_size, font);
    bt.descent     = font_ascent(font_size, font);
    bt.line_height = font_height(font_size, font);
    
    bt.glyph_scale = scale_for_font_size(font_size, font);
    
    // /////////////// //


    // ADD FIRST LINE //
    Body_Text_Line first_line = {0};
    first_line.start_byte = 0;
    first_line.start_cp = 0;

    array_add(bt.lines, first_line);
    // /// //


    // GO THROUGH GLYPHS AND DETECT LINE BREAKS //
    u8 *at  = text.data;
    u8 *end = text.data + text.length;

    v2 pp = a.p;
    pp.x += start_x;

    int prev_cp = 0;
    int cp_index = 0;

    int whitespace_or_hyphen_cp_index = -1; // Absolute index.
    u8 *whitespace_or_hyphen_end = NULL;
    
    while(at < end)
    {
        // Get next codepoint //
        u8 *cp_start = at;
        int cp = eat_codepoint(&at);
        u8 *cp_end = at;

        if(cp == '\r') continue;
        
        // Set these to add a linebreak. //
        u8 *next_line_start = NULL;
        int next_line_start_cp_index;
        
        if(cp == '\n') {
            // Obviously
            next_line_start = cp_end;
            next_line_start_cp_index = cp_index + 1;
        }
        else
        {
            if(is_whitespace(cp) || cp == '-')
            {
                whitespace_or_hyphen_cp_index = cp_index;
                whitespace_or_hyphen_end      = cp_end;
            }
            
            // Calculate where the glyph would end up, and see if its x1 > the rect's x1.
            // if so, add a line break before it.
            float x0 = pp.x;
            float x1 = codepoint_x1(cp, pp, font_size, font, bt.glyph_scale, &pp, prev_cp);

            if(x1 > a_x1)
            {
                if(whitespace_or_hyphen_cp_index != -1) // Break at last whitespace
                {
                    next_line_start = whitespace_or_hyphen_end;
                    next_line_start_cp_index = whitespace_or_hyphen_cp_index + 1;
                }
                else // There are no whitespaces on this line. So we need to break the word here. Sad.
                {
//                    next_line_x0 = a.x + pp.x - x0;
                    next_line_start = cp_start;
                    next_line_start_cp_index = cp_index;
                }
            }

        }

        if(next_line_start && multiline)
        {
            // Add linebreak => Add new Body_Text_Line
            Body_Text_Line new_line;
            new_line.start_byte = next_line_start - bt.text.data;
            new_line.start_cp = next_line_start_cp_index;

            array_add(bt.lines, new_line);

            // Prepare position for next line //
            pp.x = a.x;
            pp.y += bt.line_height;

            // Reset line state //
            whitespace_or_hyphen_cp_index = -1;

            // Set current codepoint to first codepoint on the new line //
            at = next_line_start;
            cp_index = next_line_start_cp_index;
            prev_cp = '\n';
        }
        else
        {
            // Set prev_cp and increment cp_index before next loop //
            prev_cp = cp;
            cp_index++;
        }
    }
    bt.end_p = pp - a.p;
    bt.end_cp = prev_cp;
    // /////////////////////// //

    // Set total number of codepoints in text //
    bt.num_codepoints = cp_index;

    return bt;
}

inline
float body_text_height(Body_Text *bt)
{
    return bt->line_height * bt->lines.n;
}

float body_text_line_width(int line_index, Body_Text *bt, Font *fonts)
{
    Assert(line_index >= 0 && line_index < bt->lines.n);
    
    auto &line = bt->lines[line_index];

    v2 pp = V2_ZERO;    
    u8 *at = bt->text.data + line.start_byte;
    u8 *end;
    if(line_index == bt->lines.n-1) end = bt->text.data + bt->text.length;
    else  end = bt->text.data + bt->lines[line_index+1].start_byte;

    Font *font = &fonts[bt->font];

    float x0 = pp.x;
    float min_x = FLT_MAX;
    float max_x = 0;

    int prev_cp = 0;
    int cp_index = 0;
    while(at < end)
    {
        int cp = eat_codepoint(&at);
        if(cp != '\n' && cp != '\r'){

            Rect glyph_a = codepoint_area(cp, pp, bt->font_size, font, bt->glyph_scale, &pp, prev_cp);
            if (cp_index == 0)
            {
                float delta = x0 - glyph_a.x;
                glyph_a.x += delta;
                pp.x += delta;
            }
            
            if(glyph_a.x < min_x)             min_x = glyph_a.x;
            if(glyph_a.x + glyph_a.w > max_x) max_x = glyph_a.x + glyph_a.w;
                         
            cp_index++; // @Robustness: Should this be outside branch?
        }                     
        prev_cp = cp;
    }

    return max(0, max_x - min_x);
}

inline
float body_text_line_width(int line_index, Body_Text *bt, Graphics *gfx)
{
    return body_text_line_width(line_index, bt, gfx->fonts);
}


// NOTE: clip_rect is a pointer just so we can set it to NULL.
void draw_body_text(String text, Font_Size font_size, Font_ID font, Rect a, v4 color, Graphics *gfx, H_Align h_align = HA_LEFT, Rect *clip_rect = NULL,
                    Body_Text *_bt = NULL, V_Align v_align = VA_TOP, float start_x = 0)
{
    Body_Text body_text = create_body_text(text, a, font_size, font, gfx->fonts, start_x);

    v2 p = a.p;
    if(v_align == VA_BOTTOM)
        p.y += a.h - body_text_height(&body_text);
    else if(v_align == VA_CENTER)
        p.y += a.h * 0.5f - body_text_height(&body_text) * 0.5f;
    
    draw_body_text(&body_text, p, color, gfx, h_align, clip_rect); //TODO @Robustness: @Incomplete: h_align should be taken into account in create_body_text

    if(_bt) *_bt = body_text;
}
