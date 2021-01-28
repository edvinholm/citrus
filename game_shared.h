
/*
  Shared Game
  IMPORTANT to give enums etc explicit values so they don't end up being different on client/server
*/

const int MAX_ENTITIES_PER_ROOM = 256;

typedef double World_Time;

// TODO @Robustness, @Norelease: We should be able to skip values here, so ITEM_NONE_OR_NUM would not be guaranteed to be correct.
enum Item_Type_ID
{
    ITEM_CHAIR,
    ITEM_BED,
    ITEM_TABLE,
    ITEM_PLANT,

    ITEM_NONE_OR_NUM
};

struct Item_Type
{
    v3s volume;
    v4 color;

    String name;
};

Item_Type item_types[] = { // TODO @Cleanup: Put visual stuff in client only.
    { {2, 2, 4}, {0.6, 0.1, 0.6, 1.0}, STRING("Chair") }, // Chair
    { {3, 6, 1}, {0.1, 0.6, 0.6, 1.0}, STRING("Bed") },   // Bed
    { {2, 4, 2}, {0.6, 0.6, 0.1, 1.0}, STRING("Table") }, // Table
    { {1, 1, 3}, {0.3, 0.8, 0.1, 1.0}, STRING("Plant") }, // Plant
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
    ENTITY_ITEM
};

typedef u64 Entity_ID;
const Entity_ID NO_ENTITY = 0;

struct S__Entity
{
    Entity_ID id;
    
    v3 p;
    Entity_Type type;

    union {
        struct { // item
            Item item;

            union {
                struct {
                    World_Time t_on_plant;
                    float grow_progress_on_plant;
                } plant;
            };
        } item_e;
    };
};
  

// TODO @Cleanup: Move from header.
//                This should be available to both server and client though.
void update_entity_item(S__Entity *e, double world_t)
{
    Assert(e->type == ENTITY_ITEM);

    auto *item = &e->item_e.item;
    switch(item->type) {
        case ITEM_PLANT: {
            auto *plant = &item->plant;
            auto *state = &e->item_e.plant;

            plant->grow_progress = state->grow_progress_on_plant + (world_t - state->t_on_plant) * (1.0f / 60.0f);
        } break;
    }
}


// TODO @Cleanup: Move from header.
//                This should be available to both server and client though.
S__Entity create_item_entity(Item *item, double world_t)
{
    S__Entity e = {0};
    e.type = ENTITY_ITEM;
    e.item_e.item = *item;

    switch(e.item_e.item.type) {
        case ITEM_PLANT: {
            auto *plant_e = &e.item_e.plant;
            plant_e->t_on_plant = world_t;
            plant_e->grow_progress_on_plant = item->plant.grow_progress;
        } break;
    }

    return e;
}



// Must fit in a s8.
enum Tile_Type {
    TILE_SAND = 0,
    TILE_GRASS = 1,
    TILE_STONE = 2,
    TILE_WATER = 3,

    TILE_NONE_OR_NUM = 4
};

typedef s8 Tile;

const u64 room_size_x = 32;
const u64 room_size_y = 32;
const u64 room_size   = room_size_x * room_size_y;

typedef s32 Room_ID; // Only positive numbers are allowed room IDs.

// Contains what's shared between client and server.
struct S__Room
{
    World_Time t;
    Tile tiles[room_size_x * room_size_y];
};
void reset(S__Room *room) {
    memset(room->tiles, 0, sizeof(room->tiles));
}
