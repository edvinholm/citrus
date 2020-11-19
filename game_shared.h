
/*
  Shared Game
  IMPORTANT to give enums etc explicit values so they don't end up being different on client/server
*/


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
