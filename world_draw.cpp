

void draw_static_world_geometry(Room *room, Graphics *gfx)
{
    v4 sand  = {0.6,  0.5,  0.4, 1.0f}; 
    v4 grass = {0.25, 0.6,  0.1, 1.0f};
    v4 stone = {0.42, 0.4, 0.35, 1.0f};
    v4 water = {0.1,  0.3, 0.5,  0.9f};

    v4 wall = {0.99, 0.99, 0.99, 1};

    auto *tiles = room->tiles;

    float tile_s = 1;
    
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
                case TILE_WALL:  {
                    color = &wall;

                    if(x >= room_size_x-2 || y >= room_size_y-2)
                        z = 7;
                    else
                        z = 1.25f;
                    
                } break;
                default: Assert(false); break;
            }

#if DEBUG
            if(tweak_bool(TWEAK_COLOR_TILES_BY_POSITION)) {
                v4 foo = { (float)x / room_size_x, (float)y / room_size_y, 0, 1 };
                if(tile != TILE_WATER) color = &foo;
            }
#endif
      
            if(tile == TILE_WATER) {
                draw_quad({tile_a.x, tile_a.y, z}, {1, 0, 0}, {0, 1, 0}, sand, gfx);

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
                if(x == 0 || tiles[y * room_size_x + x - 1] != tile || x >= room_size_x-2) {
                    auto cc = *color;
                    draw_quad({tile_a.x, tile_a.y, z}, {0, 1, 0}, {0, 0, -z}, cc, gfx);
                }
                // Y
                if(y == 0 || tiles[(y-1) * room_size_x + x] != tile || y >= room_size_y-2) {
                    auto cc = *color;
                    draw_quad({tile_a.x, tile_a.y, z}, {0, 0, -z}, {1, 0, 0}, cc, gfx);
                }
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

                v3 d2 = { 0, 0, height };
                
                // @Hack: To get draw_quad to calculate normal correctly......... Also for correct back-face culling i guess??
                if(comp == 1) swap(&d1, &d2);
                
                draw_quad(origin, d1, d2, color, gfx);
            
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

        { Scoped_Push(gfx->vertex_buffer, &gfx->universal_vertex_buffer);
            
            draw_static_world_geometry(room, gfx);
        }

        wgfx->static_opaque_vao.vertex1 = gfx->universal_vertex_buffer.n;
        wgfx->static_opaque_vao.needs_push = true;
        // //// //

        room->static_geometry_up_to_date = true;
    }
}


void draw_aabb(AABB bbox, Graphics *gfx)
{    
    v3 p0 = bbox.p;
    v3 p1 = bbox.p + bbox.s;
        
    v3 a = { p0.x, p0.y, p0.z };
    v3 b = { p0.x, p1.y, p0.z };
    v3 c = { p0.x, p0.y, p1.z };
    v3 d = { p0.x, p1.y, p1.z };
        
    v3 e = { p1.x, p0.y, p0.z };
    v3 f = { p1.x, p1.y, p0.z };
    v3 g = { p1.x, p0.y, p1.z };
    v3 h = { p1.x, p1.y, p1.z };

    float line_w = .1f;

    draw_line(a, b, -V3_X, line_w, C_GREEN, gfx);
    draw_line(a, c, -V3_X, line_w, C_GREEN, gfx);
    draw_line(b, d, -V3_X, line_w, C_GREEN, gfx);
    draw_line(c, d, -V3_X, line_w, C_GREEN, gfx);
        
    draw_line(e, f,  V3_X, line_w, C_GREEN, gfx);
    draw_line(e, g,  V3_X, line_w, C_GREEN, gfx);
    draw_line(f, h,  V3_X, line_w, C_GREEN, gfx);
    draw_line(g, h,  V3_X, line_w, C_GREEN, gfx);
        
    draw_line(a, e, -V3_Y, line_w, C_GREEN, gfx);
    draw_line(c, g, -V3_Y, line_w, C_GREEN, gfx);
        
    draw_line(b, f,  V3_Y, line_w, C_GREEN, gfx);
    draw_line(d, h,  V3_Y, line_w, C_GREEN, gfx);
}

void draw_entity(Entity *e, double world_t, Room *room, Client *client, Graphics *gfx, bool hovered = false, bool cannot_be_placed = false)
{
    auto *s_e = static_cast<S__Entity *>(e);


    Mesh_ID mesh = MESH_NONE_OR_NUM;
    
    float shadow_factor = 0.90f;
    
    v3 center;
    Quat q;
    get_entity_transform(s_e, world_t, room, &center, &q);

    
    float scale = 1.0f;
    
    v3 volume = V3_ONE;

    v4 base_color = V4_ONE;

    float fill = 0;
    v4 fill_color = C_WHITE;

    
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

        if(item_type->flags & ITEM_IS_LQ_CONTAINER)
        {
            float capacity = liquid_container_capacity(item) / 10.0f;

            float liquid_amount;
            liquid_container_lerp(&e->item_e.lc0, &e->item_e.lc1, e->item_e.lc_t0, e->item_e.lc_t1, world_t, &liquid_amount);
            
            fill = (capacity > 0) ? liquid_amount / capacity : 0;
            fill_color = liquid_color(item->liquid_container.liquid);
        }

        if(item->type == ITEM_CHESS_BOARD) {
            volume.z = 0.1f;
        }
    }
    else if(e->type == ENTITY_PLAYER)
    {
        auto *player_e = &e->player_e;
        
        volume = { 1.2, 1.2, player_entity_height };
        
        if(player_e->sitting_on != NO_ENTITY) {
            base_color = { 0.93, 0.52, 0.72, 1.0 };
        } else {
            base_color = { 0.93, 0.72, 0.52, 1.0 };
        }

        if(e->player_local.is_me) {
            adjust_saturation(&base_color, 2.0f);
            base_color.rgb *= 0.8f;
        }

        int num_put_down_volumes;
        auto *put_down_volumes = find_player_put_down_volumes<ALLOC_TMP>(e, world_t, room, &num_put_down_volumes, current_user(client));
        if(num_put_down_volumes > 0)
        {
            _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
            for(int i = 0; i < num_put_down_volumes; i++)
            {
                AABB vol = put_down_volumes[i];
                draw_quad(vol.p + V3_Z * 0.001f, { vol.s.x, 0, 0 }, {0, vol.s.y, 0 }, { 0.2, 0, 0.5, 1 }, gfx);
            }
        }
        
        if(tweak_bool(TWEAK_SHOW_PLAYER_PATHS))
        {
            _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
            
            v4 path_color = { 0.08, 0.53, 0.90, 1 };
                
            for(int i = 0; i < player_e->walk_path_length; i++)
            {
                v3 p = player_e->walk_path[i];
                
                if(i > 0)  draw_line(player_e->walk_path[i-1], p, V3_Z, 0.2f, path_color, gfx);
                
                draw_quad(p - V3_XY * 0.25f, V3_X * 0.7f, V3_Y * 0.7f, path_color, gfx);
            }
        }
    }

    if(hovered) {
        adjust_saturation(&base_color, 1.2f);
        base_color.rgb *= 1.3f;
    }

    if(cannot_be_placed) {
        base_color.r *= 1.5f;
        base_color.g /= 1.5f;
        base_color.b /= 1.5f;
    }

    mesh = mesh_for_entity(e);

    if(mesh != MESH_NONE_OR_NUM) {
        draw_mesh(mesh, scale_matrix(V3_ONE * scale) * rotation_matrix(q) * translation_matrix(center), &gfx->world_render_buffer.opaque, gfx, 0.0f, base_color);
    }

    
    // DEBUG AABB //
    if(tweak_bool(TWEAK_SHOW_ENTITY_BOUNDING_BOXES)) {
        _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
        AABB bbox = entity_aabb(e, center, q);

        draw_aabb(bbox, gfx);
    }
    // ////////// //



    if(tweak_bool(TWEAK_SHOW_ENTITY_ACTION_POSITIONS)) {
        if(e->id == room->selected_entity) {

            _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
            
            Entity_Action dummy_action = {0};
            dummy_action.type = ENTITY_ACT_PICK_UP;
            auto action_positions = entity_action_positions(e, &dummy_action, player_entity_hands_zoffs, world_t, room);
            for(int i = 0; i < action_positions.n; i++) {
                auto p = V3(action_positions[i]) + V3_Z * 0.05f;
                draw_quad(p - V3_XY * 0.15f, V3_X * 0.3f, V3_Y * 0.3f, C_MAGENTA, gfx);
            }
        }
    }
            
    
    
    _OPAQUE_WORLD_VERTEX_OBJECT_(rotation_matrix(q) * translation_matrix(center));

    // Forward vector
    if(tweak_bool(TWEAK_SHOW_ENTITY_FORWARD_VECTORS))
        draw_line(V3_ZERO + V3_Z * 1.0f, V3_ZERO + V3_Z * 1.0f + V3_X * 3.0f, V3_Z, 0.5f, C_CYAN, gfx);
    
    
    v3 origin  = V3_ZERO;
    origin.xy -= volume.xy * 0.5f;
        
    // PREVIEW ANIMATION //
    if(e->is_preview) {
        origin.z += 0.25f + sin(world_t) * 0.1f;
    }
    // ---

    if(fill > 0) {
        v3 fill_s = volume;
        fill_s.z *= fill;
        draw_cube_ps(origin, fill_s, fill_color, gfx);

        volume.z -= fill_s.z;
        origin.z += fill_s.z;
    }
    if(mesh == MESH_NONE_OR_NUM)
        draw_cube_ps(origin, volume, base_color, gfx);


    
    if(e->type == ENTITY_PLAYER)
    {    
        float head_size = 1.8f;
        
        v3 head_p = V3_ZERO;
        head_p.z += volume.z;
        head_p.x -= head_size * 0.5f;
        head_p.y -= head_size * 0.5f;

        v3 head_center = head_p + V3_ONE * head_size * 0.5f;

        v4 head_color = base_color;
        head_color.rgb *= 0.94f;
        
        draw_cube_ps(head_p, V3_ONE * head_size, head_color, gfx);


        // hair
        v3 hair_s = { head_size * 0.8f, head_size * 1.1f, head_size * 0.6f };
        v3 hair_p = head_center + V3(-0.55f, -0.55f, -0.05f) * head_size;
        draw_cube_ps(hair_p, hair_s, { 0.20, 0.09, 0.02, 1 }, gfx);

        // HANDS Z //
        if(tweak_bool(TWEAK_SHOW_PLAYER_ENTITY_PARTS)) {
            v3 pp = center;
            pp.x -= 1.4;
            pp.y -= 1.4;
            pp.z += player_entity_hands_zoffs;
            draw_quad(pp, V3_X * 2.8, V3_Y * 2.8, C_FUCHSIA, gfx);    
        }
    }
    else if(e->type == ENTITY_ITEM)
    {
        if(e->item_e.item.type == ITEM_CHESS_BOARD)
        {
            auto *board = &e->item_e.chess_board;

            Scoped_Push(gfx->transform, translation_matrix(V3_Z * (origin.z + volume.z + 0.001f)));

            Rect a = { origin.xy, volume.xy };        
            draw_chess_board(board, a, gfx);
        }
        
    }
    
}

void draw_world(Room *room, double world_t, m4x4 projection, Client *client, Graphics *gfx,
                Entity_ID hovered_entity = NO_ENTITY, Ray mouse_ray = {0})
{
    maybe_update_static_room_vaos(room, gfx);
    
#if 1

    v4 sand  = {0.6,  0.5,  0.4, 1.0f}; 
    v4 grass = {0.25, 0.6,  0.1, 1.0f};
    v4 stone = {0.42, 0.4, 0.35, 1.0f};
    v4 water = C_WATER;

    auto *tiles = room->tiles;

    const float tile_s = 1;
    
        // REMEMBER: Some things are in the static vao.
    
    // ENTITIES //
    {
        for(int i = 0; i < room->entities.n; i++) {
            auto *e = &room->entities[i];
            bool hovered = (e->id == hovered_entity);
            draw_entity(e, world_t, room, client, gfx, hovered);
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
