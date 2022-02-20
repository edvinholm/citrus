
// NOTE: area_tile0 and area_tile1 is the area if walker_volume is { 1, 1, 1 }.
void set_walk_flags_on_area(Walk_Flags flags, v3 area_p0, v3 area_p1, v3s walker_volume, Walk_Map *map)
{
    v3 p0 = area_p0 - V3(walker_volume) * 0.5f; // @Norelease: When we do z, don't subtract half here, but add the full z to P1. Because origin of walker is at bottom of volume.
    v3 p1 = area_p1 + V3(walker_volume) * 0.5f;

    v3 tp0 = volume_tp_from_p(p0, walker_volume); // This is the "last"  tile "before" the unwalkable area we can walk on
    v3 tp1 = volume_tp_from_p(p1, walker_volume); // This is the "first" tile "after"  the unwalkable area we can walk on

    v3s tile0 = V3S(compfloor(tp0));
    v3s tile1 = V3S(compfloor(tp1));

    for(int yy = tile0.y + 1; yy < tile1.y; yy++) {
        if(yy < 0 || yy >= room_size_y) continue;
                    
        for(int xx = tile0.x + 1; xx < tile1.x; xx++) {
            if(xx < 0 || xx >= room_size_x) continue;
                        
            map->nodes[yy * room_size_x + xx].flags |= flags; // @Speed: flags are spread out (map->nodes[*] has many members that we skip over)
        }
    }
}

// NOTE: This function does not take into consideration that the walker_volume might rotate.
//         If you for example have a walker with different x and y size, that can rotate,
//         you should probably pass x = max(x,y), y = max(x,y).
void generate_walk_map(Room *room, v3s walker_volume, Walk_Map *_map)
{
    Function_Profile();
    
    memset(_map, 0, sizeof(Walk_Map));
    
    for(int t = 0; t < room_size_x * room_size_y; t++) {
        
        if(room->tiles[t] == TILE_WALL ||
           room->tiles[t] == TILE_WATER ||
           room->tiles[t] == TILE_NONE_OR_NUM)
        {

            auto x = t % room_size_x;
            auto y = t / room_size_y;

            v3 p0 = { (float)x, (float)y, 0 };
            set_walk_flags_on_area(UNWALKABLE, p0, p0 + V3_ONE, walker_volume, _map);            
        }
    }

    // Fill in squares occupied by entities.
    for(int i = 0; i < room->num_entities; i++) {
        auto *e = &room->entities[i];
        if(e->type != ENTITY_ITEM && e->type != ENTITY_DECOR) continue;
        if(e->held_by != NO_ENTITY) continue;
        
        auto hitbox = entity_hitbox(e, room->t, room);
        v3 p1 = compround(hitbox.base.p + hitbox.base.s);

        set_walk_flags_on_area(UNWALKABLE, hitbox.base.p, p1, walker_volume, _map);
    }


#if 0 // This prints the walk map to the console.
    for(int y = 0; y < room_size_y; y++) {
        for(int x = 0; x < room_size_x; x++) {
            Debug_Print("%c", (_map[y * room_size_x + x] & UNWALKABLE) ? 'X' : '_');
        }

        Debug_Print("\n");
    }
#endif
    
}
