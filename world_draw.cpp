

void draw_static_world_geometry(Room *room, Graphics *gfx)
{
    v4 sand  = {0.6,  0.5,  0.4, 1.0f}; 
    v4 grass = {0.25, 0.6,  0.1, 1.0f};
    v4 stone = {0.42, 0.4, 0.35, 1.0f};
    v4 water = {0.1,  0.3, 0.5,  0.9f};

    auto *tiles = room->shared.tiles;

    float tile_s = 1;
    
    for(int y = 0; y < room_size_y; y++) {
        for(int x = 0; x < room_size_x; x++) {
                
            Rect tile_a = { tile_s * x, tile_s * y, tile_s, tile_s };

            auto tile = tiles[y * room_size_x + x];
                
            v4 *color = NULL;
            float z = -0.0001f;
            switch(tile) {
                case TILE_SAND:  color = &sand; break;
                case TILE_GRASS: color = &grass; break;
                case TILE_STONE: color = &stone; break;
                case TILE_WATER: color = &stone; z = -1; break;
                default: Assert(false); break;
            }

            if(tile == TILE_WATER) {
                draw_quad({tile_a.x, tile_a.y,   z}, {1, 0, 0}, {0, 1, 0}, sand, gfx);

                // WEST
                if(x != 0 && tiles[y * room_size_x + x - 1] != TILE_WATER)
                    draw_quad({tile_a.x, tile_a.y,   z}, {0, 0,-z}, {0, 1, 0}, *color, gfx);
                // NORTH
                if(y != 0 && tiles[(y-1) * room_size_x + x] != TILE_WATER)
                    draw_quad({tile_a.x, tile_a.y,   z}, {0, 0,-z}, {1, 0, 0}, *color, gfx);
                // EAST
                if(x != room_size_x-1 && tiles[y * room_size_x + x + 1] != TILE_WATER)
                    draw_quad({tile_a.x+1, tile_a.y, z}, {0, 0,-z}, {0, 1, 0}, *color, gfx);
                // SOUTH
                if(y != room_size_y-1 && tiles[(y+1) * room_size_x + x] != TILE_WATER)
                    draw_quad({tile_a.x, tile_a.y+1, z}, {0, 0,-z}, {1, 0, 0}, *color, gfx);
            }
            else if(color){
                draw_quad({tile_a.x, tile_a.y, z}, {1, 0, 0}, {0, 1, 0}, *color, gfx);
            }
        }
    }

    // GROUND SIDES //
    float shadow_factor = 0.90f;
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
    if(e->shared.type != ENTITY_ITEM) return;

    update_entity_item(&e->shared, world_t);
    
    auto *item = &e->shared.item_e.item;
    Item_Type *item_type = item_types + item->type;
    v3 origin = e->shared.p;
    origin.xy -= item_type->volume.xy * 0.5f;

    float shadow_factor = 0.90f;
            
    v4 side_color_1 = item_type->color;
    side_color_1.xyz *= shadow_factor;
    v4 side_color_2 = item_type->color;
    side_color_2.xyz *= 1.0f / shadow_factor;

    // PREVIEW ANIMATION //
    if(e->is_preview) {
        origin.z += 0.25f + sin(world_t) * 0.1f;
    }
    // ---

    float height = item_type->volume.z;
    Assert(e->shared.type == ENTITY_ITEM);
    if(item->type == ITEM_PLANT)
    {
        auto *plant = &e->shared.item_e.item.plant;
        height *= min(1.0f, plant->grow_progress);
    }

#if 0 
    // Bottom
    draw_quad(origin, { (float)item_type->volume.x, 0, 0 }, { 0, (float)item_type->volume.y, 0 }, item_type->color, gfx);
#endif

    // Sides //
    draw_quad(origin, { (float)item_type->volume.x, 0, 0 }, { 0, 0, (float)height }, side_color_1, gfx);
    draw_quad(origin, { 0, (float)item_type->volume.y, 0 }, { 0, 0, (float)height }, side_color_2, gfx);

    // Top //
    v3 top_origin = origin;
    top_origin.z += height;
    draw_quad(top_origin, { (float)item_type->volume.x, 0, 0 }, { 0, (float)item_type->volume.y, 0 }, item_type->color, gfx);
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

    auto *tiles = room->shared.tiles;

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
