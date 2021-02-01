
/*
  Shared Game
  IMPORTANT to give enums etc explicit values so they don't end up being different on client/server
*/

const int MAX_ENTITIES_PER_ROOM = 256;
const int MAX_CHAT_MESSAGES_PER_ROOM = 32;

typedef double World_Time;

// TODO @Robustness, @Norelease: We should be able to skip values here, so ITEM_NONE_OR_NUM would not be guaranteed to be correct.
enum Item_Type_ID
{
    ITEM_CHAIR,
    ITEM_BED,
    ITEM_TABLE,
    ITEM_PLANT,
    ITEM_MACHINE,

    ITEM_NONE_OR_NUM
};

struct Item_Type
{
    v3s volume;
    v4 color;

    String name;
};

enum Entity_Action_Type
{
    ENTITY_ACT_PICK_UP = 1,

    ENTITY_ACT_HARVEST = 2,

    ENTITY_ACT_SET_POWER_MODE = 3
};

struct Entity_Action
{
    Entity_Action_Type type;

    union {
        struct {
            bool set_to_on;
        } set_power_mode;
    };
};


Item_Type item_types[] = { // TODO @Cleanup: Put visual stuff in client only.
    { {2, 2, 4}, {0.6, 0.1, 0.6, 1.0}, STRING("Chair") },
    { {3, 6, 1}, {0.1, 0.6, 0.6, 1.0}, STRING("Bed") }, 
    { {2, 4, 2}, {0.6, 0.6, 0.1, 1.0}, STRING("Table") },
    { {1, 1, 3}, {0.3, 0.8, 0.1, 1.0}, STRING("Plant") },
    { {2, 2, 2}, {0.3, 0.5, 0.5, 1.0}, STRING("Machine") }
};
static_assert(ARRLEN(item_types) == ITEM_NONE_OR_NUM);

typedef u64 Item_ID;
const Item_ID NO_ITEM = 0;

struct Item {
    Item_ID id;
    Item_Type_ID type;

    union {
        struct {
            float grow_progress;
        } plant;
    };
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

const double player_walk_speed = 4.2f;
static_assert(player_walk_speed > 0);


enum Player_Action_Type
{
    PLAYER_ACT_ENTITY,
    PLAYER_ACT_WALK
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
    };
};


struct S__Entity
{
    Entity_ID id;
    
    Entity_Type type;

    union {
        struct {
            v3 p;
            
            Item item;

            union {
                struct {
                    World_Time t_on_plant;
                    float grow_progress_on_plant;
                } plant;
                
                struct {
                    World_Time start_t; // NOTE: Machine is on if start_t > stop_t.
                    World_Time stop_t;
                } machine;
            };
        } item_e;

        struct {
            User_ID user_id;

            World_Time walk_t0;
            u16 walk_path_length; // IMPORTANT: This must always be >= 2.
            v3  walk_path[8]; // @Norelease: 8 is not enough. Should this be stored on the entity? How much is enough?

            u8 action_queue_length;
            Player_Action action_queue[16];
            
        } player_e;
    };
};
  

// Must fit in an s8.
enum Tile_Type {
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


const u64 room_size_x = 32;
const u64 room_size_y = 32;
const u64 room_size   = room_size_x * room_size_y;

typedef s32 Room_ID; // Only positive numbers are allowed room IDs.

// Contains what's shared between client and server.
struct S__Room
{
    World_Time t; // t should only be on server. Client computes this (system_time + time_offset). Send this only in INIT_ROOM
    Tile tiles[room_size_x * room_size_y];
    
    int num_chat_messages;
    Chat_Message chat_messages[MAX_CHAT_MESSAGES_PER_ROOM];
};
void reset(S__Room *room) {
    memset(room->tiles, 0, sizeof(room->tiles));
}
