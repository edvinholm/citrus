
/*
  Shared Game
  IMPORTANT to give enums etc explicit values so they don't end up being different on client/server
*/

const int MAX_ENTITIES_PER_ROOM = 256;


// TODO @Robustness, @Norelease: We should be able to skip values here, so ITEM_NONE_OR_NUM would not be guaranteed to be correct.
enum Item_Type_ID
{
    ITEM_CHAIR = 0,
    ITEM_BED,
    ITEM_TABLE,

    ITEM_NONE_OR_NUM
};

struct Item_Type
{
    v3s volume;
    v4 color;
};
Item_Type item_types[] = { // TODO @Cleanup: Put visual stuff in client only.
    { {2, 2, 4}, {0.6, 0.1, 0.6, 1.0} }, // Chair
    { {3, 6, 1}, {0.1, 0.6, 0.6, 1.0} }, // Bed
    { {2, 4, 2}, {0.6, 0.6, 0.1, 1.0} }, // Table
};
static_assert(ARRLEN(item_types) == ITEM_NONE_OR_NUM);

enum Entity_Type
{
    ENTITY_ITEM
};

struct S__Entity
{
    v3 p;
    Entity_Type type;

    union {
        struct { // item
            Item_Type_ID item_type;
        };
    };
};
  


// Must fit in a s8.
enum Tile_Type {
    TILE_SAND = 0,
    TILE_GRASS = 1,
    TILE_STONE = 2,
    TILE_WATER = 3,

    TILE_NONE_OR_NUM = 4
};

typedef s8 Tile;

const u64 room_size_x = 16;
const u64 room_size_y = 16;
const u64 room_size   = room_size_x * room_size_y;

typedef s32 Room_ID; // Only positive numbers are allowed room IDs.

// Contains what's shared between client and server.
struct S__Room
{
    Tile tiles[room_size_x * room_size_y];
};
void reset(S__Room *room) {
    memset(room->tiles, 0, sizeof(room->tiles));
}
