

//@Jai: @Speed: We could do baked versions of this where sides_to_do is constant.
//IMPORTANT: This DOES NOT set the texture.
// NOTE: What is referred to as the 'slice' here, is the center rect created by the four cuts.
// NOTE: Scale obviously only works with slicing.
void draw_sprite_frame(Sprite_Frame frame, Rect rect, v2 texture_size, Graphics *gfx, Sprite_Frame *slicing = NULL, float scale = 1.0,
                       int sides_to_do = ALL_DIRECTIONS)
{
    Assert(slicing || floats_equal(scale, 1.0f)); // Scale obviously only works with slicing.
    
    v2 uvs[3*2];

    v2 map_s = texture_size;

    // Map UVs ( For convenience :-) )
    v2 muv0 = V2_ZERO;
    v2 muv1 = V2_ONE;
    
    // Frame UVs
    v2 fuv0 = compdiv(frame.p0, map_s);
    v2 fuv1 = compdiv(frame.p1, map_s);
    v2 fuv_s = fuv1 - fuv0;
    
    // Rect positions
    v2 rp0 = rect.p;
    v2 rp1 = rect.p + rect.s;
    v2 r_s = rp1 - rp0;
    
    // Slice UVs
    v2 suv0;
    v2 suv1;
    
    // Slice positions
    v2 sp0;
    v2 sp1;
    
    if(!slicing)
    {
        // Slice UVs
        suv0 = fuv0;
        suv1 = fuv1;

        // Slice positions
        sp0 = rp0;
        sp1 = rp1;
    }
    else
    {
        // Slice UVs
        suv0 = fuv0 + compdiv(slicing->p0, map_s);
        suv1 = fuv0 + compdiv(slicing->p1, map_s);
    
        // Frame size in pixels
        v2 f_s = frame.p1 - frame.p0; //scale?
    
        // Slice positions
        sp0 = rp0 + slicing->p0 * scale;
        sp1 = rp1 - (f_s - slicing->p1) * scale;

        if(!(sides_to_do & UP))     sp0.y = rp0.y;
        if(!(sides_to_do & DOWN))   sp1.y = rp1.y;
        if(!(sides_to_do & LEFT))   sp0.x = rp0.x;
        if(!(sides_to_do & RIGHT))  sp1.x = rp1.x;
        

        if(sides_to_do & UP)
        {
            if(sides_to_do & LEFT)
            {
                // Top left
                quad_uvs(uvs, fuv0, suv0);
                draw_rect_pp(rp0, sp0, gfx, uvs);
            }

            // Top
            quad_uvs(uvs, {suv0.x, fuv0.y}, {suv1.x, suv0.y});
            draw_rect_pp({sp0.x, rp0.y}, {sp1.x, sp0.y}, gfx, uvs);

            if(sides_to_do & RIGHT)
            {    
                // Top right
                quad_uvs(uvs, {suv1.x, fuv0.y}, {fuv1.x, suv0.y});
                draw_rect_pp({sp1.x, rp0.y}, {rp1.x, sp0.y}, gfx, uvs);
            }
        }
        
        if(sides_to_do & DOWN)
        {
            if(sides_to_do & LEFT)
            {
                // Bottom left
                quad_uvs(uvs, {fuv0.x, suv1.y}, {suv0.x, fuv1.y});
                draw_rect_pp({rp0.x, sp1.y}, {sp0.x, rp1.y}, gfx, uvs);
            }
    
            // Bottom
            quad_uvs(uvs, {suv0.x, suv1.y}, {suv1.x, fuv1.y});
            draw_rect_pp({sp0.x, sp1.y}, {sp1.x, rp1.y}, gfx, uvs);
            
            if(sides_to_do & RIGHT)
            {
                // Bottom right
                quad_uvs(uvs, suv1, fuv1);
                draw_rect_pp(sp1, rp1, gfx, uvs);
            }
        }
    
        if(sides_to_do & LEFT)
        {
            // Left
            quad_uvs(uvs, {fuv0.x, suv0.y}, {suv0.x, suv1.y});
            draw_rect_pp({rp0.x, sp0.y}, {sp0.x, sp1.y}, gfx, uvs);
        }
        
        if(sides_to_do & RIGHT)
        {
            // Right
            quad_uvs(uvs, {suv1.x, suv0.y}, {fuv1.x, suv1.y});
            draw_rect_pp({sp1.x, sp0.y}, {rp1.x, sp1.y}, gfx, uvs);
        }
    }
    
    // Center
    quad_uvs(uvs, suv0, suv1);
    draw_rect_pp(sp0, sp1, gfx, uvs);    
}

//IMPORTANT: This DOES NOT set the texture.
// NOTE: Scale obviously only works with slicing.
inline
void draw_sprite(Sprite &sprite, Rect rect, v2 texture_size, Graphics *gfx, float scale = 1.0f, int sides_to_do = ALL_DIRECTIONS)
{
    draw_sprite_frame(sprite.frame, rect, texture_size, gfx, (sprite.has_slicing) ? &sprite.slicing : NULL, scale, sides_to_do);
}









/*
  Procs below are using Sprite_IDs, which we don't have in this project yet. -EH, 2020-10-29
*/

#if 0


//IMPORTANT: This DOES set the texture, and UNSETS it afterwards.
// NOTE: Scale obviously only works with slicing.
inline
void draw_sprite(Sprite_ID sprite_id, Rect rect, float scale = 1.0f, int sides_to_do = ALL_DIRECTIONS)
{
    auto tex = SPRITE_TEXTURES[sprite_id];
    _TEXTURE_(tex);
    draw_sprite(SPRITES.sprites[sprite_id], rect, TEXTURES.sizes[tex], scale, sides_to_do);
}


//IMPORTANT: This DOES set the texture, and UNSETS it afterwards.
inline
void draw_sprite(Sprite_ID sprite_id, v2 center, v2 scale = V2_ONE)
{
    auto tex = SPRITE_TEXTURES[sprite_id];
    _TEXTURE_(tex);
    
    Sprite &sprite = SPRITES.sprites[sprite_id];
    
    v2 s = compmul(sprite.frame.p1 - sprite.frame.p0, scale);
    v2 p = center - s/2.0f;
    
    draw_sprite(sprite, rect(p, s), TEXTURES.sizes[tex]);
}

inline
void draw_sprite(Sprite_ID sprite_id, v2 center, float scale)
{
    draw_sprite(sprite_id, center, V2_ONE * scale);
}


//IMPORTANT: This DOES set the texture, and UNSETS it afterwards.
//NOTE: angle is in radians
inline
void draw_sprite_rotated(Sprite_ID sprite_id, v2 center, double angle, float scale = 1.0f, v2 rotation_point_offset = V2_ZERO)
{
    _ROTATE_(angle, center + rotation_point_offset);
    draw_sprite(sprite_id, center, scale);
}



inline
void draw_circle_slice_of_sprite(Sprite_ID sprite, v2 center, double angle, float scale = 1.0f)
{
    _TEXTURE_(SPRITE_TEXTURES[sprite]);
    
    v2 uv0, uv1;
    sprite_uvs(sprite, &uv0, &uv1);
    disc_segment(center, sprite_size(sprite).w * 0.5 * scale,
                 angle, -PI / 2.0, 36, uv0, uv1);
}




// Draw sprite with a = area().
inline
void sprite_a(Sprite_ID id, float scale = 1.0f)
{
    draw_sprite(id, area(), scale);
}

// Draw sprite with center = center_of(area())
inline
void sprite_c(Sprite_ID id, float scale = 1.0f)
{
    draw_sprite(id, center(), scale);
}



#endif 
