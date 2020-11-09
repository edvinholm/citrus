


inline
v2 sprite_size(Sprite &sprite)
{
    return sprite.frame.p1 - sprite.frame.p0;
}

#if 0 // We don't have Sprite_IDs yet -EH, 2020-10-29
inline
v2 sprite_size(Sprite_ID sprite)
{
    return sprite_size(SPRITES.sprites[sprite]);
}
#endif

inline
void sprite_frame_uvs(Sprite_Frame &frame, Texture_ID texture, Graphics *gfx, v2 *_uv0, v2 *_uv1)
{
    float texture_w = gfx->textures.sizes[texture].w;
    float texture_h = gfx->textures.sizes[texture].h;
    
    *_uv0 = { frame.p0.x / texture_w, frame.p0.y / texture_h };
    *_uv1 = { frame.p1.x / texture_w, frame.p1.y / texture_h };
}


#if 0 // We don't have Sprite_IDs yet -EH, 2020-10-29
inline
void sprite_uvs(Sprite_ID sprite_id, /*int frame_index = 0,*/ v2 *_uv0, v2 *_uv1)
{
    return sprite_frame_uvs(SPRITES.sprites[sprite_id].frame, SPRITE_TEXTURES[sprite_id], _uv0, _uv1);
}
#endif

// NOTE: uv0 and uv1 are the top-left and bottom-right corners of the center rectangle.
// IMPORTANT: Coordinates are in pixels!
inline
Sprite_Frame sprite_slicing(v2 p0, v2 p1)
{
    Sprite_Frame slicing = {0};
    slicing.p0 = p0;
    slicing.p1 = p1;
    return slicing;
}

Sprite_Map sprite_map(Pixel *pixels, u32 w, u32 h)
{
    Sprite_Map map = {0};

    map.pixels = pixels;
    map.w = w;
    map.h = h;

    return map;
}

void create_gpu_texture_for_sprite_map(Sprite_Map *map, Graphics *gfx, GPU_Error_Code *_error_code)
{
    Texture_ID texture = map->texture;
    gfx->textures.ids[texture] = create_texture(map->pixels, map->w, map->h, &gfx->textures.params[texture], _error_code);
}

// NOTE: Will NOT create a gpu texture.
Sprite_Map create_sprite_map(u32 w, u32 h, Texture_ID texture, Allocator_ID allocator, Graphics *gfx, bool clear /* = true */)
{
    Sprite_Map map = {0};
    
    map.pixels = allocate_bitmap(w, h, allocator);
    map.w = w;
    map.h = h;
    
    gfx->textures.ids  [texture] = 0;
    gfx->textures.sizes[texture] = V2S(w, h);
    gfx->textures.exists[texture] = true;
    
    map.texture = texture;

    if(clear)
        memset(map.pixels, 0, sizeof(*map.pixels) * map.w * map.h);
    

#if DEBUG
    // @Robustness: What if the sprite map's pixels pointer changes??
    gfx->textures.pixels[texture] = map.pixels;
#endif

    return map;
}

//TODO @Incomplete
//TODO @Incomplete
//TODO @Incomplete
//TODO @Incomplete
bool find_empty_location_in_sprite_map(Sprite_Map &map, u32 sprite_w, u32 sprite_h,
                                       u32 *_x, u32 *_y)
{
    //@JAI: using Map;

    //To avoid tearing:
    sprite_w += 1;
    sprite_h += 1;
    
    if(map.xx <= map.w - sprite_w)
    {
        
        v2u result = {map.xx, map.yy};
        
        if(map.row_h < sprite_h)
        {
            if(map.yy <= map.h-sprite_h)
                map.row_h = sprite_h;
            else
                return false;
        }
        map.xx += sprite_w;

        #if DEBUG && 0
        float MapSize = map.w * map.h;
        float Used = map.yy * map.w + map.xx;
        float PercentageUsed = (Used/MapSize) * 100.0f;
        printf("Sprite map %.2f%% full.\n", PercentageUsed);
        #endif

        *_x = result.x;
        *_y = result.y;
        return true;
    }
    else
    {
        map.yy += map.row_h;
        map.xx = 0;
        map.row_h = 0;

        return find_empty_location_in_sprite_map(map, sprite_w, sprite_h, _x, _y);
    }
}

//NOTE: _output should be at least 12 floats long
void uv_coordinates_for_sprite_frame(Sprite_Map &map, Sprite_Frame frame, float *_output)
{
    //These are coordinates from 0.0 to 1.0
    float u0 = (float)frame.x0/(float)map.w;
    float v0 = (float)frame.y0/(float)map.h;
    float u1 = (float)frame.x1/(float)map.w;
    float v1 = (float)frame.y1/(float)map.h;

    _output[0] = u0;
    _output[1] = v0;
    _output[2] = u1;
    _output[3] = v0;
    _output[4] = u1;
    _output[5] = v1;
    
    _output[6] = u0;
    _output[7] = v0;
    _output[8] = u0;
    _output[9] = v1;
    _output[10] = u1;
    _output[11] = v1;
}


void write_pixels_to_sprite_map(Sprite_Map *map, Pixel *pixels, u32 pixels_w, u32 pixels_h,
                                u32 origin_x, u32 origin_y)
{
    write_pixels_to_bitmap(pixels, pixels_w, pixels_h,
                           map->pixels, map->w, map->h,
                           origin_x, origin_y);
}


void update_sprite_map_texture_if_needed(Sprite_Map *map, Graphics *gfx)
{
    if(!map->needs_update) return;

    upload_texture_to_gpu(gfx->textures.ids[map->texture], map->w, map->h, map->pixels, gfx->textures.params[map->texture]);
    gfx->textures.exists[map->texture] = true;
    
    map->needs_update = false;
}




#if 0 // We don't have Sprite_IDs yet -EH, 2020-10-29
String sprite_name(Sprite_ID sprite_id)
{
    Assert(sprite_id < NUM_SPRITES);
    return SPRITE_NAMES[sprite_id];
}

void init_sprite(int id_, Sprite_Frame frame)
{
    Sprite_ID id = (Sprite_ID)id_;
    Assert(id < NUM_SPRITES);

    SPRITES.sprites[id] = sprite(frame, NULL);

#if DEBUG
    SPRITES.DEBUG_initted[id] = true;
#endif    
}

void init_sliced_sprite(int id_, v2 frame_p0, v2 frame_p1, v2 slice_p0, v2 slice_p1)
{
    Sprite_ID id = (Sprite_ID)id_;
    Assert(id < NUM_SPRITES);
    
    Sprite_Frame frame;
    frame.p0 = frame_p0;
    frame.p1 = frame_p1;
    Sprite_Frame slicing = sprite_slicing(slice_p0, slice_p1);
    SPRITES.sprites[id] = sprite(frame, &slicing);

#if DEBUG
    SPRITES.DEBUG_initted[id] = true;
#endif
}

#endif


void init_sprites(Graphics *gfx) //@JAI: #run
{
    TIMED_FUNCTION;
    
    // TODO Zero(gfx->sprites);

    // TODO :
    //GENERATED_init_sprites();

    
/* Example:
   
    init_sliced_sprite(SPRITE_UI_BUTTON,         V2(4, 5), V2(50, 50), V2(12, 6), V2(12, 12));
    init_sliced_sprite(SPRITE_UI_BUTTON_HOVERED, V2(54, 5), V2(50, 50), V2(12, 6), V2(12, 12));
    init_sliced_sprite(SPRITE_UI_BUTTON_PRESSED, V2(104, 5), V2(50, 50), V2(12, 6), V2(12, 12));
    
    init_sliced_sprite(SPRITE_UI_TEXTBOX,        V2(3, 83), V2(50, 50), V2(12, 12), V2(12, 12));
    init_sliced_sprite(SPRITE_UI_TEXTBOX_ACTIVE, V2(54, 83), V2(50, 50), V2(12, 12), V2(12, 12));

    init_sliced_sprite(SPRITE_UI_WINDOW, V2(104, 55), V2(50, 50), V2(9, 32), V2(9, 9));
*/

}
