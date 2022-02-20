
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
    
    ITEM_APPLE_TREE,
    ITEM_WHEAT,
    
    ITEM_MACHINE,
    ITEM_WATERING_CAN,
    ITEM_CHESS_BOARD,
    ITEM_BARREL,
    
    ITEM_BLENDER,
    ITEM_BLENDER_CONTAINER,
    
    ITEM_FRUIT,
    ITEM_BOX,
    ITEM_FILTER_PRESS,
    ITEM_STOVE,
    ITEM_GRINDER,

    ITEM_TOILET,

    ITEM_NONE_OR_NUM
};

enum Item_Type_Flag_: u8 {
};

typedef u8 Item_Type_Flags;
static_assert(sizeof(Item_Type_Flags) == sizeof(Item_Type_Flag_));

// Bitfield.
enum Substance_Form: u8
{
    SUBST_LIQUID = 0x1,
    SUBST_NUGGET = 0x2,

    SUBST_NONE = 0
};

struct Item_Type
{
    v3s volume;
    v4 color;

    String name;

    Substance_Form container_forms;
    
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
    ENTITY_ACT_SIT_OR_UNSIT = 7,

    // Bed
    ENTITY_ACT_SLEEP = 8,

    // Toilet
    ENTITY_ACT_USE_TOILET = 9,

    // Seed containers
    ENTITY_ACT_PLANT = 10,


    
    ENTITY_ACT_MOVE = 11, // This is the same as PICK_UP + PUT_DOWN.
};

struct Entity_Action
{
    Entity_Action_Type type;
    
    union {
        struct {
            v3 p;
            Quat q;
        } move;
        
        struct {
            bool set_to_on;
        } set_power_mode;

        struct {
            bool unsit;
        } sit_or_unsit;

        Chess_Action chess;

        struct {
            bool impacting_bladder; // Not synced, used by server only (2021-04-04)
            bool impacting_bowel;   // Not synced, used by server only (2021-04-04)
        } use_toilet;

        struct {
            v3 tp; // @Security: Make sure aligned to grid on server.
        } plant;
    };
};


Item_Type item_types[] = { // TODO @Cleanup: Put visual stuff in client only.
    { {2, 2, 4}, { 0.6,  0.1,  0.6, 1.0}, STRING("Chair"), SUBST_NONE },
    { {7, 4, 3}, { 0.1,  0.6,  0.6, 1.0}, STRING("Bed"),   SUBST_NONE },
    { {4, 6, 2}, { 0.6,  0.6,  0.1, 1.0}, STRING("Table"), SUBST_NONE },
    
    { {4, 4, 8}, { 0.3,   0.8,  0.1, 1.0}, STRING("Apple Tree"), SUBST_NONE },
    { {1, 1, 3}, { 1.00, 0.71, 0.26, 1.0}, STRING("Wheat"),      SUBST_NONE },
    
    { {2, 2, 2}, { 0.3,  0.5,  0.5, 1.0}, STRING("Machine"),      SUBST_NONE },
    { {1, 2, 1}, {0.73, 0.09, 0.00, 1.0}, STRING("Watering Can"), SUBST_LIQUID },
    { {2, 2, 1}, { 0.1,  0.1,  0.1, 1.0}, STRING("Chess Board"),  SUBST_NONE },
    { {2, 2, 3}, { 0.02, 0.2, 0.12, 1.0}, STRING("Barrel"),       SUBST_LIQUID },
    
    { {2, 2, 2}, {0.35, 0.81, 0.77, 1.0}, STRING("Blender"),           SUBST_NONE },
    { {2, 2, 1}, {0.88, 0.24, 0.99, 1.0}, STRING("Blender Container"), SUBST_LIQUID },
    
    { {1, 1, 1}, {0.74, 0.04, 0.04, 1.0}, STRING("Fruit"), SUBST_NONE },

    { {2, 1, 1}, {0.85, 0.71, 0.55, 1.0}, STRING("Box"), SUBST_NUGGET },
    { {3, 7, 4}, {0.05, 0.15, 0.66, 1.0}, STRING("Filter Press"), SUBST_NONE },
    { {3, 3, 3}, {0.97, 0.96, 0.95, 1.0}, STRING("Stove"), SUBST_NONE },
    { {2, 2, 3}, {0.15, 0.66, 0.24, 1.0}, STRING("Grinder"), SUBST_NONE },
    { {3, 2, 4}, {1.0,   1.0,  1.0, 1.0}, STRING("Toilet"),  SUBST_NONE }
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
    LQ_BREAD_DOUGH, // Not really a liquid...
    LQ_FLOUR,       // Not really a liquid...
    LQ_NONE_OR_NUM
};

String liquid_names[] = {
    STRING("WATER"),
    STRING("YEAST WATER"),
    STRING("BREAD DOUGH"),
    STRING("FLOUR")
};
static_assert(ARRLEN(liquid_names) == LQ_NONE_OR_NUM);

typedef u32 Liquid_Fraction;

struct Liquid
{
    Liquid_Type type;
    union {
        struct {
            Liquid_Fraction yeast;
            Liquid_Fraction nutrition;
        } yeast_water;

        struct {
            float bake_progress;
        } bread_dough;
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
        { 1.00, 0.95, 0.62, 0.9 },
        { 0.95, 0.86, 0.69, 1.0 },
        { 0.99, 0.97, 0.94, 1.0 }
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




enum Nugget_Type: u8
{
    NUGGET_YEAST,
    NUGGET_SEEDS,

    NUGGET_NONE_OR_NUM
};

String nugget_names[] = {
    STRING("YEAST"),
    STRING("SEEDS")
};
static_assert(ARRLEN(nugget_names) == NUGGET_NONE_OR_NUM);

#if !(SERVER)
v4 nugget_colors[] = {
    { 1.00, 0.95, 0.62, 1.0f },
    {  0.6, 0.65, 0.42, 1.0f }
};
static_assert(ARRLEN(nugget_colors) == NUGGET_NONE_OR_NUM);
#endif

enum Seed_Type: u16 {
    SEED_APPLE,
    SEED_WHEAT,

    SEED_NONE_OR_NUM
};

String seed_names[] = {
    STRING("APPLE"),
    STRING("WHEAT")
};
static_assert(ARRLEN(seed_names) == SEED_NONE_OR_NUM);

struct Nugget {
    Nugget_Type type;

    union {
        Seed_Type seed_type;
    };
};

bool equal(Nugget *a, Nugget *b) {
    // @Robustness: @Norelease Is this a good idea? Should be fine if we don't have any floats in Nugget(???????????? what about padding???????)
    static_assert(sizeof(*a) == sizeof(*b));
    return (memcmp(a, b, sizeof(*a)) == 0);
}



typedef u32 Substance_Amount;

struct Substance
{
    Substance_Form form;
    
    union {
        Liquid liquid;
        Nugget nugget;
    };
};

bool equal(Substance *a, Substance *b) {
    if(a->form != b->form) return false;
    switch(a->form) { // @Jai: #complete
        case SUBST_LIQUID: return equal(&a->liquid, &b->liquid);
        case SUBST_NUGGET: return equal(&a->nugget, &b->nugget);

        default: Assert(false); return false;
    }
}

struct Substance_Container
{
    Substance        substance;
    Substance_Amount amount;
};

bool equal(Substance_Container *a, Substance_Container *b) {
    if(a->amount != b->amount) return false;
    if(a->amount == 0) return true;
    return equal(&a->substance, &b->substance);
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

    Substance_Container container;
};

bool equal(Item *a, Item *b)
{
    if(a->id != b->id)     return false;
    if(a->type != b->type) return false;
    return true;
}


enum Decor_Type_ID: u32
{
    DECOR_FENCE,
    DECOR_STREET_LIGHT,
    DECOR_AWNING,
    DECOR_FOUNTAIN,
    DECOR_DOOR,
    DECOR_WINDOW,
    DECOR_FLOWER_BOX_WALL,

    DECOR_SIGN_CHESS,

    DECOR_NONE_OR_NUM
};

struct Decor_Type {
    v3s volume;
};

Decor_Type decor_types[] = {
    { 1, 4, 2  }, // Fence
    { 3, 1, 13 }, // Street light
    { 2, 2, 2 },
    { 4, 4, 5 }, // Fountain
    { 1, 4, 8 }, // Door
    { 1, 4, 5 },  // Window
    { 1, 4, 2 },  // Flower Box (Wall)
    { 1, 10, 4 }  // Sign: Chess
};
static_assert(ARRLEN(decor_types) == DECOR_NONE_OR_NUM);

struct Decor {
    Decor_Type_ID type;
    v3 p;
    Quat q;
};


enum Entity_Type
{
    ENTITY_ITEM,
    ENTITY_PLAYER,
    ENTITY_DECOR
};

typedef u64 Entity_ID;
const Entity_ID NO_ENTITY = 0;

// PLAYER STUFF //
v3s player_entity_volume = { 2, 2, 4 };

const double player_walk_speed = 4.2f;
static_assert(player_walk_speed > 0);
// ------------ //


typedef u32 Player_Action_ID;

enum Player_Action_Type
{
    PLAYER_ACT_ENTITY,
    PLAYER_ACT_WALK,
    PLAYER_ACT_PUT_DOWN, // This is a player action, and not an entity action, because the item we want to put down might not be attached to an entity at the time we enqueue the action.
    PLAYER_ACT_PLACE_FROM_INVENTORY
};

struct Player_Action
{
    Player_Action_Type type;
    u8 step;

    World_Time reach_t;
    World_Time update_t;
    World_Time end_t;
    World_Time end_retry_t;

    bool dequeue_requested;

    union {
        struct {
            v3 p1;
        } walk;

        struct {
            Entity_ID     target;
            Entity_Action action;
        } entity;

        struct {
            v3 p;
            Quat q;
        } put_down;

        struct {
            Item_ID item;
            v3 p;
            Quat q;
        } place_from_inventory;
    };
};


struct Machine
{
    World_Time t_on_recipe_begin; // NOTE: We are currently "doing" a recipe if t_on_recipe_begin + recipe_duration > t.
    World_Time recipe_duration;
                    
    //NOTE: These are only valid if t_on_recipe_begin + recipe_duration > t
    Static_Array<Entity_ID, MAX_RECIPE_INPUTS> recipe_inputs;
    Static_Array<bool,      MAX_RECIPE_INPUTS> recipe_input_used_as_container;
    Static_Array<Entity_ID, MAX_RECIPE_INPUTS> recipe_outputs;
    //--
};

enum Need
{
    NEED_ENERGY,
    NEED_SLEEP,
    NEED_BLADDER,
    NEED_BOWEL,

    NEED_NONE_OR_NUM
};

String need_names[] = {
    STRING("ENERGY"),
    STRING("SLEEP"),
    STRING("BLADDER"),
    STRING("BOWEL")
};
static_assert(ARRLEN(need_names) == NEED_NONE_OR_NUM);

#if DEBUG
const float need_speed_multiplier = 1.0f;
#else
const float need_speed_multiplier = 1.0f;
#endif

const float default_need_speeds[] = {
    -0.00083f * need_speed_multiplier,
    -0.00037f * need_speed_multiplier,
    -0.00074f * need_speed_multiplier,
    -0.00043f * need_speed_multiplier
};
static_assert(ARRLEN(default_need_speeds) == NEED_NONE_OR_NUM);

// @BadName: The needs must be less than or equal to these values before the player
//           can actively satisfy them. Like using the toilet for example.
float need_limits[] = {
    1.0f,
    0.25f,
    0.5f,
    0.25f
};
static_assert(ARRLEN(need_limits) == NEED_NONE_OR_NUM);

struct Needs
{
    float values[NEED_NONE_OR_NUM];
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

                struct { Machine machine; } blender;
                struct { Machine machine; } filter_press;
                struct { Machine machine; } grinder;

                Chess_Board chess_board;
            };

            struct {
                
                World_Time t0;
                World_Time t1;

                Substance_Container c0;
                Substance_Container c1;
                
            } container;
            
        } item_e;

        struct {
            User_ID user_id;

            World_Time walk_t0;
            u16 walk_path_length; // IMPORTANT: walk_path_length must always be >= 2.
            v3 *walk_path;

            u8 action_queue_length;
            Player_Action    action_queue[16]; // @Norelease @SecurityMini: Some actions, like chess moves, you want to be private and not downloaded by other players.
            Player_Action_ID action_ids[ARRLEN(action_queue)];
            bool             action_queue_pauses[ARRLEN(action_queue)+1]; // true at [0] means pause before action_queue[0].

            Entity_ID is_on;
            bool laying_down_instead_of_sitting; // Only valid if is_on != NO_ENTITY

            World_Time needs_t0;
            Needs needs0;
            float need_change_speeds[NEED_NONE_OR_NUM];
            
        } player_e;

        
        Decor decor;
        
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

    // These are not predicted
    Needs needs;
    // --
    
    v3 p;
    Item held_item; // No item if .type == ITEM_NONE_OR_NUM.
    
    Entity_ID sitting_on;
    Entity_ID laying_on;
};

enum Tile: s8 {
    TILE_SAND    = 0,
    TILE_GRASS   = 1,
    TILE_CONCRETE_TILES_2X2 = 2,
    TILE_WATER   = 3,
    TILE_ASPHALT = 5,
    TILE_WOOD    = 6,

    TILE_WALL = 4,
    
    TILE_NONE_OR_NUM = 8
};



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


