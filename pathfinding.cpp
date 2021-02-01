

enum Walk_Flag {
    WALKABLE = 0x0001
};

typedef u8 Walk_Mask;


// NOTE: _map should point to a buffer big enough to hold room_size_x * room_size_y Walk_Masks.
void generate_walk_map(Room *room, Walk_Mask *_map)
{
    Walk_Mask *at = _map;
    for(int t = 0; t < room_size_x * room_size_y; t++) {
        *at = 0;
        if(room->tiles[t] != TILE_WALL && room->tiles[t] != TILE_WATER)
            (*at) |= WALKABLE;

        at++;
    }

    // Fill in squares occupied by entities.
    for(int i = 0; i < room->num_entities; i++) {
        auto *e = &room->entities[i];
        if(e->type != ENTITY_ITEM) continue;
        
        AABB bbox = entity_aabb(e, room->t);
        v3 p1 = bbox.p + bbox.s;
        
        v3s tp0 = { (s32)floorf(bbox.p.x), (s32)floorf(bbox.p.y), (s32)floorf(bbox.p.z) }; // If the bbox overlaps only a little bit, make the whole tile not walkable.
        v3s tp1 = { (s32)ceilf(p1.x), (s32)ceilf(p1.y), (s32)ceilf(p1.z) };

        for(int y = tp0.y; y < tp1.y; y++)
        {
            if(y < 0) continue;
            if(y >= room_size_y) break;

            for(int x = tp0.x; x < tp1.x; x++) {
                if(x < 0) continue;
                if(x >= room_size_x) break;

                _map[y * room_size_x + x] &= ~(WALKABLE);
            }
        }
    }

}
