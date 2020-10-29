
    
struct Pixel;

struct Sprite_Map
{
    Pixel *pixels; // Can be null...

    union
    {
        v2u s;
        struct
        {
            u32 w;
            u32 h;
        };
    };
    
    u32 row_h;
    u32 xx;
    u32 yy;

    Texture_ID texture;

    bool needs_update;
};

struct Sprite_Frame
{
    union
    {
        v2 p0; //In pixels
        struct
        {
            float x0;
            float y0;
        };
    };
    union
    {
        v2 p1; //In pixels
        struct
        {
            float x1;
            float y1;
        };
    };
};


struct Sprite
{
    Sprite_Frame frame; // We can have multiple frames here later.
    bool has_slicing;
    Sprite_Frame slicing;
};

Sprite sprite(Sprite_Frame frame, Sprite_Frame *slicing = NULL)
{
    Sprite result = {0};
    result.frame = frame;
    if(slicing)
    {
        result.has_slicing = true;
        result.slicing = *slicing;
    }
    
    return result;
}


#define MAX_NUM_SPRITESHEET_SPRITES 128;

void init_sprite(int id_, Sprite_Frame frame);
void init_sliced_sprite(int id_, v2 frame_p0, v2 frame_s, v2 slice_p0_cut, v2 slice_p1_cut);


/* TODO: #include "spritesheets.cpp"

struct Sprite_Catalog
{
    Sprite sprites[NUM_SPRITES];

#if DEBUG
    bool DEBUG_initted[NUM_SPRITES];
#endif
};

Sprite_Catalog SPRITES = {};
*/



//@JAI: Remove these.

bool find_empty_location_in_sprite_map(Sprite_Map &map, u32 sprite_w, u32 sprite_h,
                                       u32 *_x, u32 *_y);

void write_pixels_to_sprite_map(Sprite_Map *map, Pixel *pixels, u32 pixels_w, u32 pixels_h,
                                u32 origin_x, u32 origin_y);

void create_gpu_texture_for_sprite_map(Sprite_Map *map);


struct Graphics;

Sprite_Map create_sprite_map(u32 w, u32 h, Texture_ID texture, Allocator_ID allocator, Graphics *gfx, bool clear = true);
