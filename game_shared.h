
/*
  Shared Game
  IMPORTANT to give enums etc explicit values so they don't end up being different on client/server
*/

const int MAX_ENTITIES_PER_ROOM = 256;
const int MAX_CHAT_MESSAGES_PER_ROOM = 32;

typedef double World_Time;

const float player_entity_height      = 3.4f;
const   s32 player_entity_hands_zoffs = 2;

// TODO @Robustness, @Norelease: We should be able to skip values here, so ITEM_NONE_OR_NUM would not be guaranteed to be correct.
enum Item_Type_ID
{
    ITEM_CHAIR,
    ITEM_BED,
    ITEM_TABLE,
    ITEM_PLANT,
    ITEM_MACHINE,
    ITEM_WATERING_CAN,
    ITEM_CHESS_BOARD,
    ITEM_BARREL,
    
    ITEM_BLENDER,
    ITEM_BLENDER_CONTAINER,
    
    ITEM_FRUIT,

    ITEM_NONE_OR_NUM
};

enum Item_Type_Flag_: u8 {
    ITEM_IS_LQ_CONTAINER = 0x01
};

typedef u8 Item_Type_Flags;
static_assert(sizeof(Item_Type_Flags) == sizeof(Item_Type_Flag_));

struct Item_Type
{
    v3s volume;
    v4 color;

    String name;

    Item_Type_Flags flags;
};

enum Entity_Action_Type
{
    ENTITY_ACT_PICK_UP = 1,
    ENTITY_ACT_PLACE_IN_INVENTORY = 2,

    // Plant
    ENTITY_ACT_HARVEST = 3,
    ENTITY_ACT_WATER = 4,

    // Machine
    ENTITY_ACT_SET_POWER_MODE = 5,

    // Chess
    ENTITY_ACT_CHESS = 6,

    // Chair
    ENTITY_ACT_SIT_OR_UNSIT = 7
};

struct Entity_Action
{
    Entity_Action_Type type;

    union {
        struct {
            bool set_to_on;
        } set_power_mode;

        struct {
            bool unsit;
        } sit_or_unsit;

        Chess_Action chess;
    };
};


Item_Type item_types[] = { // TODO @Cleanup: Put visual stuff in client only.
    { {2, 2, 4}, { 0.6,  0.1,  0.6, 1.0}, STRING("Chair"), 0 },
    { {3, 6, 1}, { 0.1,  0.6,  0.6, 1.0}, STRING("Bed"),   0 },
    { {2, 4, 2}, { 0.6,  0.6,  0.1, 1.0}, STRING("Table"), 0 },
    { {1, 1, 3}, { 0.3,  0.8,  0.1, 1.0}, STRING("Plant"), 0 },
    { {2, 2, 2}, { 0.3,  0.5,  0.5, 1.0}, STRING("Machine"), 0 },
    { {1, 2, 1}, {0.73, 0.09, 0.00, 1.0}, STRING("Watering Can"), ITEM_IS_LQ_CONTAINER },
    { {2, 2, 1}, { 0.1,  0.1,  0.1, 1.0}, STRING("Chess Board"), 0 },
    { {2, 2, 3}, { 0.02, 0.2, 0.12, 1.0}, STRING("Barrel"), ITEM_IS_LQ_CONTAINER },
    
    { {2, 2, 2}, {0.35, 0.81, 0.77, 1.0}, STRING("Blender"), 0 },
    { {2, 2, 1}, {0.88, 0.24, 0.99, 1.0}, STRING("Blender Container"), ITEM_IS_LQ_CONTAINER },
    
    { {1, 1, 1}, {0.74, 0.04, 0.04, 1.0}, STRING("Fruit"), 0 }
};
static_assert(ARRLEN(item_types) == ITEM_NONE_OR_NUM);

struct Item_ID
{
    u64 origin; // First 32 bits is the ID of the server that created the item.
                // Last  32 bits can be used by the origin server to identify an internal origin. For example, a room server could set this to some representation of which room and where in that room the item was created.
    u64 number;
};
bool operator == (const Item_ID &a, const Item_ID &b) {
    return (a.origin == b.origin &&
            a.number == b.number);
}
bool operator != (const Item_ID &a, const Item_ID &b) {
    return !(a == b);
}

const Item_ID NO_ITEM = { 0, 0 };

enum Liquid_Type: u8 {
    LQ_WATER,
    LQ_YEAST_WATER,
    LQ_NONE_OR_NUM
};

String liquid_names[] = {
    STRING("WATER"),
    STRING("YEAST WATER")
};
static_assert(ARRLEN(liquid_names) == LQ_NONE_OR_NUM);

typedef u32 Liquid_Amount;
typedef u32 Liquid_Fraction;

struct Liquid
{
    Liquid_Type type;
    union {
        struct {
            Liquid_Fraction yeast;
            Liquid_Fraction nutrition;
        } yeast_water;
    };
};
bool equal(Liquid *a, Liquid *b) {
    // @Robustness: @Norelease Is this a good idea? Should be fine if we don't have any floats in Liquid(???????????? what about padding???????)
    static_assert(sizeof(*a) == sizeof(*b));
    return (memcmp(a, b, sizeof(*a)) == 0);
}

#if !(SERVER)
v4 liquid_color(Liquid lq) {

    if(lq.type == LQ_NONE_OR_NUM) return { 0, 0, 0, 0 };
    
    v4 base_colors[] = {
        C_WATER,
        { 1.00, 0.95, 0.62, 0.9f }
    };
    static_assert(ARRLEN(base_colors) == LQ_NONE_OR_NUM);

    Assert(lq.type >= 0 && lq.type < ARRLEN(base_colors));
    v4 color = base_colors[lq.type];

    if(lq.type == LQ_YEAST_WATER) {
        color = lerp(C_WATER, color, clamp((lq.yeast_water.yeast / 1000.0f)/0.5f));
    }
    
    return color;
}
#endif

struct Liquid_Container
{
    Liquid liquid;
    Liquid_Amount amount;
};
bool equal(Liquid_Container *a, Liquid_Container *b) {
    if(!equal(&a->liquid, &b->liquid)) return false;
    if(!floats_equal(a->amount, b->amount)) return false;
    return true;
}

struct Item {
    Item_ID id;
    Item_Type_ID type;
    User_ID owner;

    union {
        struct {
            float grow_progress;
        } plant;
    };
    
    Liquid_Container liquid_container; // If item_types[.type].flags & ITEM_IS_LQ_CONTAINER
};

bool equal(Item *a, Item *b)
{
    if(a->id != b->id)     return false;
    if(a->type != b->type) return false;
    return true;
}



enum Entity_Type
{
    ENTITY_ITEM,
    ENTITY_PLAYER
};

typedef u64 Entity_ID;
const Entity_ID NO_ENTITY = 0;

// PLAYER STUFF //
v3s player_entity_volume = { 2, 2, 4 };

const double player_walk_speed = 4.2f;
static_assert(player_walk_speed > 0);
// ------------ //



enum Player_Action_Type
{
    PLAYER_ACT_ENTITY,
    PLAYER_ACT_WALK,
    PLAYER_ACT_PUT_DOWN, // Put down held item.
    PLAYER_ACT_PLACE_FROM_INVENTORY
};

struct Player_Action
{
    Player_Action_Type type;
    World_Time next_update_t;

    union {
        struct {
            v3 p1;
        } walk;

        struct {
            Entity_ID     target;
            Entity_Action action;
        } entity;

        struct {
            v3 tp;
            Quat q;
        } put_down;

        struct {
            Item_ID item;
            v3 tp;
            Quat q;
        } place_from_inventory;
    };
};


struct S__Entity
{
    Entity_ID id;
    
    Entity_Type type;

    Entity_ID held_by;
    Entity_ID holding;

    union {
        struct {
            v3   p;
            Quat q;
            
            Item item;
            
            Entity_ID locked_by;

            union {
                struct {
                    World_Time t_on_plant;
                    float grow_progress_on_plant;
                } plant;
                
                struct {
                    World_Time start_t; // NOTE: Machine is on if start_t > stop_t.
                    World_Time stop_t;
                } machine;

                struct {
                    World_Time t_on_recipe_begin; // NOTE: We are currently "doing" a recipe if t_on_recipe_begin + recipe_duration > t.
                    World_Time recipe_duration;
                    
                    //NOTE: These are only valid if t_on_recipe_begin + recipe_duration > t
                    Static_Array<Entity_ID, MAX_RECIPE_INPUTS> recipe_inputs;
                    Entity_ID recipe_output_container;
                    //--
                    
                } blender;

                Chess_Board chess_board;
            };

            // NOTE: Only valid if item's type's flags & ITEM_IS_LQ_CONTAINER.
            World_Time lc_t0;
            World_Time lc_t1;
            Liquid_Container lc0;
            Liquid_Container lc1;
            
        } item_e;

        struct {
            User_ID user_id;

            World_Time walk_t0;
            u16 walk_path_length; // IMPORTANT: walk_path_length must always be >= 2.
            v3 *walk_path;

            u8 action_queue_length;
            Player_Action action_queue[16]; // @Norelease @SecurityMini: Some actions, like chess moves, you want to be private and not downloaded by other players.

            Entity_ID sitting_on;
            
        } player_e;
    };
};
void clear(S__Entity *e)
{
    if(e->type != ENTITY_PLAYER) return;

    dealloc(e->player_e.walk_path, ALLOC_MALLOC);
}


// TODO: Should predict some entity state too. For example, will this entity have anything
//       that needs support on its surface? If we have PICK_UPs for all the supported entities
//       queued, this should be predicted to false.... -EH, 2021-03-02

// This is what we use to predict, for example, if a particular
// action will be possible after a given queue of actions has been performed.
struct Player_State
{
    User_ID   user_id; // Don't change this :)
    Entity_ID entity_id; // Don't change this :)
    // --
    
    v3 p;
    Item held_item; // No item if .type == ITEM_NONE_OR_NUM.
    Entity_ID sitting_on;
};

enum Tile_Type: s8 {
    TILE_SAND = 0,
    TILE_GRASS = 1,
    TILE_STONE = 2,
    TILE_WATER = 3,

    TILE_WALL = 4,
    
    TILE_NONE_OR_NUM = 5
};

typedef s8 Tile;


struct Chat_Message {
    World_Time t;
    User_ID user;
    String text; // @Norelease: Make all chat message texts be allocated together.
};


typedef s32 Room_ID; // Only positive numbers are allowed room IDs.

// Contains what's shared between client and server.
struct S__Room
{
    World_Time t; // t should only be on server. Client computes this (system_time + time_offset). Send this only in INIT_ROOM
    Tile tiles[room_size_x * room_size_y];
    Walk_Map walk_map;
    
    int num_chat_messages;
    Chat_Message chat_messages[MAX_CHAT_MESSAGES_PER_ROOM];
    
};
void clear(S__Room *room) {
    
}

enum Surface_Flag_
{
    SURF_EXCLUSIVE = 0x01,
    SURF_CENTERING = 0x02 // Items are centered on this surface.
};

typedef u8 Surface_Flags;

enum Surface_Type: u8
{
    SURF_TYPE_DEFAULT = 0,
    SURF_TYPE_MACHINE_INPUT,
    SURF_TYPE_MACHINE_OUTPUT
};

struct Surface
{
    v3 p;
    v2 s;

    s32 max_height; // For entities supported by it. Zero means no limit.
    
    Surface_Flags flags;
    Surface_Type  type;
};

