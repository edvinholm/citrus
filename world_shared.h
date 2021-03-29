
enum Surface_Flag_
{
    SURF_EXCLUSIVE = 0x01,
    SURF_CENTERING = 0x02, // Items are centered on this surface.
    SURF_HAS_SPECIFIED_PICKUP_POSITION = 0x04, // (Also for PUT_DOWN). If this is set, Surface.pickup_position is valid, and that position should be used instead of the default action positions for a PICK_UP or PUT_DOWN of an object on this surface.
};

typedef u8 Surface_Flags;

enum Surface_Type: u8
{
    SURF_TYPE_DEFAULT = 0,
    SURF_TYPE_MACHINE_INPUT,
    SURF_TYPE_MACHINE_OUTPUT
};

enum Filter_Press_Surface_ID {
    FILTER_PRESS_SURF_INPUT,
    
    FILTER_PRESS_SURF_NUGGET_OUTPUT,
    FILTER_PRESS_SURF_LIQUID_OUTPUT
};

// IMPORTANT: Keep this small, because we copy it into every Support!
struct Surface_Owner_Specifics {
    union {
        Filter_Press_Surface_ID filter_press_surface_id;
    };
};

struct Surface
{
    v3 p;
    v2 s;

    s32 max_height; // For entities supported by it. Zero means no limit.
    
    Surface_Flags flags;
    Surface_Type  type;

    v3 pickup_position; // See comment for SURF_HAS_SPECIFIED_PICKUP_POSITION

    Surface_Owner_Specifics owner_specifics;
};

struct Entity_Hitbox
{
    AABB base;
    AABB exclusions[4];
    int  num_exclusions;
};
