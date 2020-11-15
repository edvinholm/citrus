
enum Tile {
    TILE_SAND,
    TILE_GRASS,
    TILE_STONE,
    TILE_WATER,

    TILE_NONE_OR_NUM
};

const u64 room_size_x = 256;
const u64 room_size_y = 256;

struct Room {
    double t;

    // @Temporary @NoRelease
    double randomize_cooldown;
    Tile tiles[room_size_x * room_size_y];
    // --
};

struct Server {
    Array<Room, ALLOC_GAME> rooms;
};

double get_time() {
    return (double)platform_performance_counter() / (double)platform_performance_counter_frequency();
}


void create_dummy_rooms(Server *server)
{
    double t = get_time();
    
    for(int i = 0; i < 8; i++) {
        Room room = {0};
        room.t = t;
        room.randomize_cooldown = random_float() * 3.0;
        array_add(server->rooms, room);
    }
}

void randomize_tiles(Tile *tiles, u64 num_tiles) {
    Tile *at = tiles;
    Tile *end = tiles + num_tiles;

    while(end-at >= 8) {
        at[0] = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
        at[1] = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
        at[2] = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
        at[3] = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
        at[4] = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
        at[5] = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
        at[6] = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
        at[7] = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
        at += 8;
    }

    while(at < end) {
        *at = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
        at++;
    }
}

void update_room(Room *room, int index) {

    Assert(room->t > 0); // Should be initialized when created
    
    double last_t = room->t;
    room->t = get_time();
    double dt = room->t - last_t;

    room->randomize_cooldown -= dt;
    while(room->randomize_cooldown <= 0) {
        randomize_tiles(room->tiles, ARRLEN(room->tiles));
        room->randomize_cooldown += 2.0;
        Debug_Print("Randomized tiles for room %d.\n", index);
    }
}

int server_entry_point(int num_args, char **arguments)
{
    Debug_Print("I am a server.\n");

    Server server = {0};
    
    create_dummy_rooms(&server);

    double t = get_time();
    while(true) {

        for(int i = 0; i < server.rooms.n; i++) {
            update_room(server.rooms.e + i, i);
        }
        
        platform_sleep_microseconds(1000);
    }
    
    return 0;
}
