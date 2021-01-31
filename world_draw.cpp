

void draw_static_world_geometry(Room *room, Graphics *gfx)
{
    v4 sand  = {0.6,  0.5,  0.4, 1.0f}; 
    v4 grass = {0.25, 0.6,  0.1, 1.0f};
    v4 stone = {0.42, 0.4, 0.35, 1.0f};
    v4 water = {0.1,  0.3, 0.5,  0.9f};
    
    v4 wall = {0.9, 0.9, 0.9, 1};

    auto *tiles = room->tiles;

    float tile_s = 1;
    
    float shadow_factor = 0.90f;
    
    for(int y = 0; y < room_size_y; y++) {
        for(int x = 0; x < room_size_x; x++) {
                
            Rect tile_a = { tile_s * x, tile_s * y, tile_s, tile_s };

            auto tile = tiles[y * room_size_x + x];
                
            v4 *color = NULL;
            float z = -0.0001f;
            switch(tile) {
                case TILE_SAND:  color = &sand;  break;
                case TILE_GRASS: color = &grass; break;
                case TILE_STONE: color = &stone; break;
                case TILE_WATER: color = &stone; z = -1; break;
                case TILE_WALL:  color = &wall;  z = 7;  break;
                default: Assert(false); break;
            }

            if(tile == TILE_WATER) {
                draw_quad({tile_a.x, tile_a.y,   z}, {1, 0, 0}, {0, 1, 0}, sand, gfx);

                // WEST
                if(x != 0 && tiles[y * room_size_x + x - 1] != tile)
                    draw_quad({tile_a.x, tile_a.y,   z}, {0, 0,-z}, {0, 1, 0}, *color, gfx);
                // NORTH
                if(y != 0 && tiles[(y-1) * room_size_x + x] != tile)
                    draw_quad({tile_a.x, tile_a.y,   z}, {0, 0,-z}, {1, 0, 0}, *color, gfx);
                // EAST
                if(x != room_size_x-1 && tiles[y * room_size_x + x + 1] != tile)
                    draw_quad({tile_a.x+1, tile_a.y, z}, {0, 0,-z}, {0, 1, 0}, *color, gfx);
                // SOUTH
                if(y != room_size_y-1 && tiles[(y+1) * room_size_x + x] != tile)
                    draw_quad({tile_a.x, tile_a.y+1, z}, {0, 0,-z}, {1, 0, 0}, *color, gfx);
            }
            else if(tile == TILE_WALL) {
                draw_quad({tile_a.x, tile_a.y, z}, {1, 0, 0}, {0, 1, 0}, *color, gfx);

                // X
                if(x == 0 || tiles[y * room_size_x + x - 1] != tile)
                    draw_quad({tile_a.x, tile_a.y,   z}, {0, 0,-z}, {0, 1, 0}, *color * shadow_factor, gfx);
                // Y
                if(y == 0 || tiles[(y-1) * room_size_x + x] != tile)
                    draw_quad({tile_a.x, tile_a.y,   z}, {0, 0,-z}, {1, 0, 0}, *color * (1.0f/shadow_factor), gfx);                
            }
            else if(color){
                draw_quad({tile_a.x, tile_a.y, z}, {1, 0, 0}, {0, 1, 0}, *color, gfx);
            }
        }
    }

    // GROUND SIDES //
    v4 side_color_base = sand;

    for(int i = 0; i < 2; i++) {

        int comp = (i == 0) ? 0 : 1;
        int c1 = ((comp == 0) ? room_size_x : room_size_y);

        v4 color = side_color_base;
        if(comp == 1)
            color.xyz *= shadow_factor;
        else
            color.xyz *= 1.0f/shadow_factor;
        
        int c0 = 0;
        for(int c = 1; c <= c1; c++)
        {
            auto prev_tile_ix = (c-1) * ((comp == 0) ? 1 : room_size_x);
            auto next_tile_ix = (c)   * ((comp == 0) ? 1 : room_size_x);
            
            bool prev_low = (tiles[prev_tile_ix] == TILE_WATER);

            bool do_draw = (c == c1);
            if(!do_draw) {
                bool next_low = (tiles[next_tile_ix] == TILE_WATER);
                do_draw = (next_low != prev_low);
            }

            if(do_draw)
            {
                float length = c - c0;

                float height = (prev_low) ? 1 : 2;

                v3 origin = { 0, 0, -2 };
                origin.comp[comp] = c0;

                v3 d1 = {0};
                d1.comp[comp] = length;
                
                draw_quad(origin, d1, {0, 0, height}, color, gfx);
            
                c0  = c;
            }
        }
    }
}


void maybe_update_static_room_vaos(Room *room, Graphics *gfx)
{
    World_Graphics *wgfx = &gfx->world;
    if(!room->static_geometry_up_to_date) {

        // OPAQUE //
        wgfx->static_opaque_vao.vertex0 = gfx->universal_vertex_buffer.n;

        push(gfx->vertex_buffer_stack, &gfx->universal_vertex_buffer);
        {
            draw_static_world_geometry(room, gfx);
        }
        pop(gfx->vertex_buffer_stack);

        wgfx->static_opaque_vao.vertex1 = gfx->universal_vertex_buffer.n;
        wgfx->static_opaque_vao.needs_push = true;
        // //// //

        room->static_geometry_up_to_date = true;
    }
}


void draw_entity(Entity *e, double world_t, Graphics *gfx)
{
    auto *s_e = static_cast<S__Entity *>(e);
    
    float shadow_factor = 0.90f;
    
    v3 origin = entity_position(s_e, world_t);
    v3 volume = V3_ONE;

    v4 base_color = V4_ONE;
    
    if(e->type == ENTITY_ITEM)
    {
        update_entity_item(s_e, world_t);
        
        auto *item = &e->item_e.item;
        Item_Type *item_type = item_types + item->type;
        
        volume = V3(item_type->volume);
        
        if(item->type == ITEM_PLANT)
        {
            auto *plant = &e->item_e.item.plant;
            volume.z *= min(1.0f, plant->grow_progress);
        }

        base_color = item_type->color;
    }
    else if(e->type == ENTITY_PLAYER)
    {
        auto *player_e = &e->player_e;
        
        volume = { 1.2, 1.2, 3.4 };
        
        if(e->player_local.is_me) {
            base_color = { 0.93, 0.52, 0.33, 1.0 };
        } else {
            base_color = { 0.93, 0.72, 0.52, 1.0 };
        }
    }
    

    // PREVIEW ANIMATION //
    if(e->is_preview) {
        origin.z += 0.25f + sin(world_t) * 0.1f;
    }
    // ---

    v4 side_color_1 = base_color;
    v4 side_color_2 = base_color;
    
    side_color_1.xyz *= shadow_factor;
    side_color_2.xyz *= 1.0f / shadow_factor;

    origin.xy -= volume.xy * 0.5f;

    // Sides //
    draw_quad(origin, { (float)volume.x, 0, 0 }, { 0, 0, (float)volume.z }, side_color_1, gfx);
    draw_quad(origin, { 0, (float)volume.y, 0 }, { 0, 0, (float)volume.z }, side_color_2, gfx);

    // Top //
    v3 top_origin = origin;
    top_origin.z += volume.z;
    draw_quad(top_origin, { (float)volume.x, 0, 0 }, { 0, (float)volume.y, 0 }, base_color, gfx);

    if(e->type == ENTITY_PLAYER)
    {
        float cube_size = 1.8;
        draw_cube_ps(top_origin - V3_XY * cube_size * 0.5f, V3_ONE * cube_size, base_color, gfx);
    }
}

void draw_world(Room *room, double system_t, m4x4 projection, Graphics *gfx)
{
    auto world_t  = system_t + room->time_offset;
    
    maybe_update_static_room_vaos(room, gfx);
    
#if 1

    v4 sand  = {0.6,  0.5,  0.4, 1.0f}; 
    v4 grass = {0.25, 0.6,  0.1, 1.0f};
    v4 stone = {0.42, 0.4, 0.35, 1.0f};
    v4 water = {0.1,  0.3, 0.5,  0.9f};

    auto *tiles = room->tiles;

    const float tile_s = 1;
    
    // OPAQUE //
    {
        _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
        // REMEMBER: Some things are in the static vao.

        for(int i = 0; i < room->entities.n; i++) {
            auto *e = &room->entities[i];
            draw_entity(e, world_t, gfx);
        }
    }

    // TODO: Static things here should be in a vao, just like the opaque static stuff.
    // TRANSLUCENT //
    {
        for(int y = 0; y < room_size_y; y++) {
            for(int x = 0; x < room_size_x; x++) {

                Rect tile_a = { tile_s * x, tile_s * y, tile_s, tile_s };

                if(tiles[y * room_size_x + x] == TILE_WATER) {

                    float surface_yoffs = -0.4f;
                    
                    v3 origin = {tile_a.x, tile_a.y, surface_yoffs};
                    float screen_z = vecmatmul_z(origin + V3(0.5, 0.5, 0), projection);

                    _TRANSLUCENT_WORLD_VERTEX_OBJECT_(M_IDENTITY, screen_z);
                    // TOP //
                    draw_quad(origin, {1, 0, 0},  {0, 1, 0}, water, gfx);

                    // WEST
                    if(x == 0)
                        draw_quad({tile_a.x, tile_a.y, surface_yoffs}, {0, 0, -1.0f - surface_yoffs}, {0, 1, 0}, water, gfx);
                    // SOUTH
                    if(y == 0)
                        draw_quad({tile_a.x, tile_a.y, surface_yoffs}, {0, 0, -1.0f - surface_yoffs}, {1, 0, 0}, water, gfx);
                }
            }
        }
        
    }

    
#endif

#if 0
    // @Temporary
    {
        _TRANSLUCENT_WORLD_VERTEX_OBJECT_(rotation_around_point_matrix(axis_rotation(V3_X, PI + cos(world_t) * (PI/16.0) / 2.0), { room_size_x / 2.0f, room_size_y / 2.0f, 2.5 }), -1);
        draw_string(STRING("Abc"), V2_ZERO, FS_10, FONT_TITLE, {1, 1, 1, 1}, gfx);
    }
#endif
}
