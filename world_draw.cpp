

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
                case TILE_WATER: color = &stone; z = -0.5; break;
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

    // Ground sides //
    float shadow_factor = 0.90f;
    v4 side_color_base = sand;
    
    v4 side_color_1 = side_color_base;
    side_color_1.xyz *= shadow_factor;
    
    v4 side_color_2 = side_color_base;
    side_color_2.xyz *= 1.0f / shadow_factor;
    
    draw_quad({0, 0, 0}, {(float)room_size_x, 0, 0}, {0, 0, -1}, side_color_1, gfx);
    draw_quad({0, 0, 0}, {0, (float)room_size_y, 0}, {0, 0, -1}, side_color_2, gfx);
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



void draw_world(Room *room, double t, m4x4 projection, Graphics *gfx)
{
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

        for(int i = 0; i < room->num_entities; i++) {
            auto *e = &room->entities[i].shared;
            if(e->type != ENTITY_ITEM) continue;

            Item_Type *item_type = item_types + e->item_type;
            v3 origin = e->p;
            origin.xy -= item_type->volume.xy * 0.5f;

            float shadow_factor = 0.90f;
            
            v4 side_color_1 = item_type->color;
            side_color_1.xyz *= shadow_factor;
            v4 side_color_2 = item_type->color;
            side_color_2.xyz *= 1.0f / shadow_factor;

#if 0 
            // Bottom
            draw_quad(origin, { (float)item_type->volume.x, 0, 0 }, { 0, (float)item_type->volume.y, 0 }, item_type->color, gfx);
#endif

            // Sides //
            draw_quad(origin, { (float)item_type->volume.x, 0, 0 }, { 0, 0, (float)item_type->volume.z }, side_color_1, gfx);
            draw_quad(origin, { 0, (float)item_type->volume.y, 0 }, { 0, 0, (float)item_type->volume.z }, side_color_2, gfx);

            // Top //
            v3 top_origin = origin;
            top_origin.z += item_type->volume.z;
            draw_quad(top_origin, { (float)item_type->volume.x, 0, 0 }, { 0, (float)item_type->volume.y, 0 }, item_type->color, gfx);
        }
    }

    // TRANSLUCENT //
    {
        for(int y = 0; y < room_size_y; y++) {
            for(int x = 0; x < room_size_x; x++) {

                Rect tile_a = { tile_s * x, tile_s * y, tile_s, tile_s };

                if(tiles[y * room_size_x + x] == TILE_WATER) {
#if 1
                    v3 origin = {tile_a.x, tile_a.y, -0.18f};
                    float screen_z = vecmatmul_z(origin + V3(0.5, 0.5, 0), projection);
                    _TRANSLUCENT_WORLD_VERTEX_OBJECT_(M_IDENTITY, screen_z);
                    draw_quad(origin, {1, 0, 0},  {0, 1, 0}, water, gfx);

#else
                    // @Temporary
                    // THIS IS A TEST OF TRANSLUCENT RENDERING OF AN OBJECT THAT OVERLAPS ITSELF...
                    // So we break it up in multiple objects.......... Is this the way to do it?
                    
                    v3 origin = {tile_a.x, tile_a.y, 0.5f + (float)cos((x * y)/50.0 + t) / 2.0f};
                    
                    // Top
                    {
                        float screen_z = vecmatmul_z(combined_matrix, origin + V3(0.5, 0.5, 0));
                        _TRANSLUCENT_WORLD_VERTEX_OBJECT_(world_transform, screen_z);
                        draw_quad(origin, {1, 0, 0},  {0, 1, 0}, {0, 0.5, 0.5, 0.65}, gfx);
                    }
                    
                    // Side towards negative X
                    {
                        float screen_z = vecmatmul_z(combined_matrix, origin + V3(0.5, 0, -0.5));
                        _TRANSLUCENT_WORLD_VERTEX_OBJECT_(world_transform, screen_z);
                        draw_quad(origin, {0, 0, -1}, {1, 0, 0}, {0.75, 0.0, 0.0, 0.75}, gfx);
                    }
                    
                    // Side towards negative Y
                    {
                        float screen_z = vecmatmul_z(combined_matrix, origin + V3(0, 0.5, -0.5));
                        _TRANSLUCENT_WORLD_VERTEX_OBJECT_(world_transform, screen_z);
                        draw_quad(origin, {0, 0, -1}, {0, 1, 0}, {0.0, 0.75, 0.0, 0.75}, gfx);
                    }
#endif

                }
            }
        }
        
    }

    
#endif

#if 1
    // @Temporary
    {
        _TRANSLUCENT_WORLD_VERTEX_OBJECT_(rotation_around_point_matrix(axis_rotation(V3_X, PI + cos(t) * (PI/16.0) / 2.0), { room_size_x / 2.0f, room_size_y / 2.0f, 2.5 }), -1);
        draw_string(STRING("Abc"), V2_ZERO, FS_10, FONT_TITLE, {1, 1, 1, 1}, gfx);
    }
#endif
}
