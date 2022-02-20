

int tile_side_height(Tile tile)
{
    switch(tile) {
        case TILE_WATER:       return 1;
        case TILE_NONE_OR_NUM: return 0;
    }
    return 2;
}

void get_uvs_for_tile_type(Tile tile, v2 *_uv0, v2 *_uv1)
{   
    int cr = 16; // Number of columns/rows
    Assert(tile >= 0 && tile < cr*cr);
    
    int cell_s = 128;

    int x_px = (tile % cr) * cell_s;
    int y_px = (tile / cr) * cell_s;

    double u0 = x_px / (double)(cr * cell_s);
    double v0 = y_px / (double)(cr * cell_s);
    
    double u1 = (x_px + cell_s) / (double)(cr * cell_s);
    double v1 = (y_px + cell_s) / (double)(cr * cell_s);

    *_uv0 = { (float)u0, (float)v0 };
    *_uv1 = { (float)u1, (float)v1 };

#if 1 // @Incomplete
    *_uv0 += V2_ONE * 2.0f * (1.0f / (cr * cell_s));
    *_uv1 -= V2_ONE * 2.0f * (1.0f / (cr * cell_s));
#endif
    
    
}

void draw_static_world_geometry(Room *room, Graphics *gfx)
{
    v4 sand    = {0.6,  0.5,  0.4, 1.0f}; 
    v4 grass   = {0.25, 0.6,  0.1, 1.0f};
    v4 stone   = {0.42, 0.4, 0.35, 1.0f};
    v4 water   = {0.1,  0.3, 0.5,  0.9f};
    v4 asphalt = {0.1,  0.1, 0.1,  1.0f};

    v4 wall = {0.99, 0.99, 0.99, 1};

    auto *tiles = room->tiles;

    float tile_s = 1;
    
    for(int y = 0; y < room_size_y; y++) {
        for(int x = 0; x < room_size_x; x++) {
                
            Rect tile_a = { tile_s * x, tile_s * y, tile_s, tile_s };

            auto tile = tiles[y * room_size_x + x];
                
            v4 color = V4_ONE;
            Texture_ID texture = TEX_NONE_OR_NUM;
            float z = -0.0001f;
            switch(tile) {
                case TILE_SAND:    color = sand;  break;
                case TILE_GRASS:   texture = TEX_TILES; break;
                case TILE_WATER:   color = stone; z = -1; break;
                case TILE_ASPHALT: color = asphalt; break;
                case TILE_CONCRETE_TILES_2X2: texture = TEX_TILES; break;
                    
                case TILE_WALL:  {
                    color = wall;

                    if(true /* @Incomplete */|| x >= room_size_x-2 || y >= room_size_y-2)
                        z = 14;
                    else
                        z = 1.25f;
                    
                } break;

                case TILE_NONE_OR_NUM: continue;
                    
                default: Assert(false); break;
            }

#if DEBUG
            if(tweak_bool(TWEAK_COLOR_TILES_BY_POSITION)) {
                v4 foo = { (float)x / room_size_x, (float)y / room_size_y, 0, 1 };
                if(tile != TILE_WATER) color = foo;
            }
#endif
      
            if(tile == TILE_WATER) {
                draw_quad({tile_a.x, tile_a.y, z}, {1, 0, 0}, {0, 1, 0}, sand, gfx);

                // WEST
                if(x != 0 && tiles[y * room_size_x + x - 1] != tile)
                    draw_quad({tile_a.x, tile_a.y,   z}, {0, 0,-z}, {0, 1, 0}, color, gfx);
                // NORTH
                if(y != 0 && tiles[(y-1) * room_size_x + x] != tile)
                    draw_quad({tile_a.x, tile_a.y,   z}, {0, 0,-z}, {1, 0, 0}, color, gfx);
                // EAST
                if(x != room_size_x-1 && tiles[y * room_size_x + x + 1] != tile)
                    draw_quad({tile_a.x+1, tile_a.y, z}, {0, 0,-z}, {0, 1, 0}, color, gfx);
                // SOUTH
                if(y != room_size_y-1 && tiles[(y+1) * room_size_x + x] != tile)
                    draw_quad({tile_a.x, tile_a.y+1, z}, {0, 0,-z}, {1, 0, 0}, color, gfx);
            }
            else if(tile == TILE_WALL) {
                draw_quad({tile_a.x, tile_a.y, z}, {1, 0, 0}, {0, 1, 0}, color, gfx);

                // X
                 if(x == 0 || tiles[y * room_size_x + x - 1] != tile || x >= room_size_x-2) {
                    auto cc = color;
                    
                    if(x == 0 || tiles[y * room_size_x + x - 1] == TILE_NONE_OR_NUM)
                        cc = C_BLACK;

                    draw_quad({tile_a.x, tile_a.y, z}, {0, 1, 0}, {0, 0, -z}, cc, gfx);
                }
                // Y
                if(y == 0 || tiles[(y-1) * room_size_x + x] != tile || y >= room_size_y-2) {
                    auto cc = color;
                    
                    if(y == 0 || tiles[(y-1) * room_size_x + x] == TILE_NONE_OR_NUM)
                        cc = C_BLACK;
                    
                    draw_quad({tile_a.x, tile_a.y, z}, {0, 0, -z}, {1, 0, 0}, cc, gfx);
                }
            }
            else {

                v2 uv0, uv1;
                get_uvs_for_tile_type(tile, &uv0, &uv1);
                
                v2 s = uv1 - uv0;

                v2 delta = { (x % 2) * s.x * .5f, (y % 2) * s.y * .5f };
                uv0 += delta;
                uv1 += delta;

                uv1 -= s * .5f;
                
                v2 uvs[6];
                quad_uvs(uvs, uv0, uv1);
                
                draw_quad({tile_a.x, tile_a.y, z}, {1, 0, 0}, {0, 1, 0}, color, gfx, uvs, texture);
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
            
            int prev_height = tile_side_height(tiles[prev_tile_ix]);

            bool do_draw = (c == c1);
            if(!do_draw) {
                int next_height = tile_side_height(tiles[next_tile_ix]);
                do_draw = (next_height != prev_height);
            }

            if(do_draw)
            {
                float height = prev_height;
                
                if(height > 0) {
                    float length = c - c0;

                    v3 origin = { 0, 0, -2 };
                    origin.comp[comp] = c0;

                    v3 d1 = {0};
                    d1.comp[comp] = length;

                    v3 d2 = { 0, 0, height };
                
                    // @Hack: To get draw_quad to calculate normal correctly......... Also for correct back-face culling i guess??
                    if(comp == 1) swap(&d1, &d2);
                
                    draw_quad(origin, d1, d2, color, gfx);
                }
                
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


void draw_aabb(AABB bbox, v4 color, Graphics *gfx)
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

    v3 normal = -gfx->camera_dir;

    draw_line(a, b, normal, line_w, color, gfx);
    draw_line(a, c, normal, line_w, color, gfx);
    draw_line(b, d, normal, line_w, color, gfx);
    draw_line(c, d, normal, line_w, color, gfx);
        
    draw_line(e, f, normal, line_w, color, gfx);
    draw_line(e, g, normal, line_w, color, gfx);
    draw_line(f, h, normal, line_w, color, gfx);
    draw_line(g, h, normal, line_w, color, gfx);
        
    draw_line(a, e, normal, line_w, color, gfx);
    draw_line(c, g, normal, line_w, color, gfx);
        
    draw_line(b, f, normal, line_w, color, gfx);
    draw_line(d, h, normal, line_w, color, gfx);
}

void draw_entity(Entity *e, double world_t, Graphics *gfx, Room *room = NULL, User *user = NULL, bool hovered = false, bool cannot_be_placed = false, float desaturation = 0.0f, bool do_debug_things = false, World_Render_Buffer *wrb = NULL)
{
    auto *s_e = static_cast<S__Entity *>(e);

    if(!wrb) wrb = &gfx->world_render_buffer;

    Mesh_ID mesh = MESH_NONE_OR_NUM;
    
    float shadow_factor = 0.90f;
    
    v3 center;
    Quat q;
    get_entity_transform(s_e, world_t, &center, &q, room);

    Entity_Hitbox hitbox = entity_hitbox(e, center, q);


    // Keep track of where lightboxes are instead, and look for them here.
    bool use_oven_lightbox = (e->type == ENTITY_ITEM && e->item_e.item.type == ITEM_STOVE);
    v3   oven_p = center; // only valid if use_oven_lightbox
    Quat oven_q = q;
    if(!use_oven_lightbox && room)
    {
        // @Norelease @Speed!!!
        for(int i = 0; i < room->entities.n; i++) {
            auto *other = &room->entities[i];
            if(other->type == ENTITY_ITEM && other->item_e.item.type == ITEM_STOVE) {
                v3 other_p;
                Quat other_q;
                get_entity_transform(other, world_t, &other_p, &other_q, room);
                Entity_Hitbox stove_hitbox = entity_hitbox(other, world_t, room);
                if(aabb_intersects_aabb(stove_hitbox.base, hitbox.base)) {
                    use_oven_lightbox = true;
                    oven_p = other_p;
                    oven_q = other_q;
                }
            }
        }
    }
    
    
    float scale = 1.0f;
    
    v3 volume = V3_ONE;

    v4 base_color = V4_ONE;

    float fill = 0;
    v4 fill_color = C_WHITE;


    if(e->type == ENTITY_DECOR)
    {
        auto *decor = &e->decor;
        auto *type  = &decor_types[decor->type];
        volume = V3(type->volume);
    }
    else if(e->type == ENTITY_ITEM)
    {
        update_entity_item(e, world_t);
        
        auto *item = &e->item_e.item;
        Item_Type *item_type = item_types + item->type;

        volume = V3(item_type->volume);
        
        if(item->type == ITEM_APPLE_TREE ||
           item->type == ITEM_WHEAT)
        {
            auto *plant = &e->item_e.item.plant;
            volume.z *= min(1.0f, plant->grow_progress);
        }

        base_color = item_type->color;

        if(item_type->container_forms != SUBST_NONE)
        {
            auto *c = &e->item_e.container;

            // @Speed: We probably do this lerp in update_entity_item too.
            float continuous_amount;
            Substance_Container container = substance_container_lerp(&c->c0, &c->c1, c->t0, c->t1, world_t, &continuous_amount);
            
            if(container.amount > 0 && container.substance.form == SUBST_LIQUID) {
                auto *l = &container.substance.liquid;
            
                float capacity = substance_container_capacity(item, SUBST_LIQUID) / 10.0f;

                fill = (capacity > 0) ? continuous_amount / capacity : 0;
                fill_color = liquid_color(container.substance.liquid);
            }
        }

        if(item->type == ITEM_CHESS_BOARD) {
            volume.z = 0.1f;
        }

        Machine *machine;
        if(is_machine(e, &machine) && machine_is_doing_recipe(machine, world_t)) {
            center += random_point_in_unit_sphere() * .1f;
        }
    }
    else if(e->type == ENTITY_PLAYER)
    {
        auto *player_e = &e->player_e;
        
        volume = { 1.2, 1.2, player_entity_height };
        
        if(player_e->is_on != NO_ENTITY) {
            if(player_e->laying_down_instead_of_sitting) {
                base_color = { 0.72, 0.93, 0.52, 1.0 };

            } else {
                base_color = { 0.93, 0.52, 0.72, 1.0 };
            }
        } else {
            base_color = { 0.93, 0.72, 0.52, 1.0 };
        }

        if(e->player_local.is_me) {
            adjust_saturation(&base_color, 2.0f);
            base_color.rgb *= 0.8f;
        }

        if(room && user)
        {
            int num_put_down_volumes;
            auto *put_down_volumes = find_player_put_down_volumes<ALLOC_TMP>(e, world_t, room, &num_put_down_volumes, user);
            if(num_put_down_volumes > 0)
            {
                _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
                for(int i = 0; i < num_put_down_volumes; i++)
                {
                    AABB vol = put_down_volumes[i];
                    draw_quad(vol.p + V3_Z * 0.001f, { vol.s.x, 0, 0 }, {0, vol.s.y, 0 }, { 0.2, 0, 0.5, 1 }, gfx);
                }
            }
        }
        
        if(do_debug_things && tweak_bool(TWEAK_SHOW_PLAYER_PATHS))
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

    mesh = mesh_for_entity(e, world_t);


    if(mesh != MESH_NONE_OR_NUM &&
       (!do_debug_things || !tweak_bool(TWEAK_HIDE_ENTITY_MESHES)))
    {
        Render_Object *mesh_obj = draw_mesh(mesh, scale_matrix(V3_ONE * scale) * rotation_matrix(q) * translation_matrix(center), &wrb->opaque, gfx, 0.0f, C_WHITE);
        if(use_oven_lightbox) { // NOTE: We don't do this on vertex objects, but we don't worry about that right now because everything will probably be meshes in the end anyway.

            v3 forward = rotate_vector(V3_X, oven_q);
            v3 left    = rotate_vector(V3_Y, oven_q);
            
            mesh_obj->lightbox_center   = oven_p + V3_Z * 1;
            mesh_obj->lightbox_radiuses = compabs((forward * .9) + (left * .95) + V3_Z * .9);

            float alpha = 1.0f + (float)sin(world_t + random_float() * 0.05f) * .2f + (-0.05f + random_float() * 0.1f);
            mesh_obj->lightbox_color    = { 1.00f, 0.81f, 0.16f, alpha };
        }

        mesh_obj->desaturation = desaturation;
    }

    
    // DEBUG AABB //
    if(do_debug_things && tweak_bool(TWEAK_SHOW_ENTITY_BOUNDING_BOXES)) {
        _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
        auto hitbox = entity_hitbox(e, center, q);

        draw_aabb(hitbox.base,      C_GREEN, gfx);
        for(int i = 0; i < hitbox.num_exclusions; i++)
            draw_aabb(hitbox.exclusions[i], C_RED, gfx);
    }
    // ////////// //

    // DEBUG ENTITY POSITION //
    if(do_debug_things && tweak_bool(TWEAK_SHOW_ENTITY_POSITIONS)) {
        _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
        float s = .2;
        draw_cube_ps(center - V3_XY * s*.5, {s, s, 10}, C_SKY, gfx);
    }
    // // //

    // DEBUG ACTION POSITIONS //
    if(do_debug_things && tweak_bool(TWEAK_SHOW_ENTITY_ACTION_POSITIONS)) {
        if(e->id == room->selected_entity && e->type == ENTITY_ITEM) {

            _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
            
            Entity_Action dummy_action = {0};
            dummy_action.type = ENTITY_ACT_PICK_UP;
            auto action_positions = entity_action_positions(e, &dummy_action, player_entity_hands_zoffs, world_t, room);
            for(int i = 0; i < action_positions.n; i++) {
                auto p = V3(action_positions[i]) + V3_Z * 0.001f;

                float point_size = .2;
                
                draw_quad(p - V3_XY * point_size * .5, V3_X * point_size, V3_Y * point_size, C_MAGENTA, gfx);

                draw_line(p - V3_X * .2, p + V3_X * .4, V3_Z, point_size * .5, C_MAGENTA, gfx);
                draw_line(p - V3_Y * .2, p + V3_Y * .4, V3_Z, point_size * .5, C_MAGENTA, gfx);
            }
        }
    }
    // // //


    push(gfx->vertex_buffer, &wrb->opaque.vertices);
    defer(pop(gfx->vertex_buffer););
    
    begin_vertex_render_object(&wrb->opaque, rotation_matrix(q) * translation_matrix(center));
    defer(end_vertex_render_object(&wrb->opaque););

    wrb->opaque.current_vertex_object.desaturation = desaturation;

    

    // DEBUG FORWARD VECTOR //
    if(do_debug_things && tweak_bool(TWEAK_SHOW_ENTITY_FORWARD_VECTORS))
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
        if(do_debug_things && tweak_bool(TWEAK_SHOW_PLAYER_ENTITY_PARTS)) {
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

void draw_world(Room *room, double world_t, double t, m4x4 projection, Client *client, Graphics *gfx,
                Entity_ID hovered_entity = NO_ENTITY, Ray mouse_ray = {0})
{
    maybe_update_static_room_vaos(room, gfx);

    v4 sand  = {0.6,  0.5,  0.4, 1.0f}; 
    v4 grass = {0.25, 0.6,  0.1, 1.0f};
    v4 stone = {0.42, 0.4, 0.35, 1.0f};
    v4 water = C_WATER;

    auto *tiles = room->tiles;

    const float tile_s = 1;

    auto *cui = &client->cui;
    
        // REMEMBER: Some things are in the static vao.
    
    // ENTITIES //
    {
        for(int i = 0; i < room->entities.n; i++) {
            auto *e = &room->entities[i];
            bool hovered = (e->id == hovered_entity);

            float desaturation = 0;
            
            // "Tool effects" //
            if(cui->current_tool == TOOL_PLANTING) {
                auto *tool = &cui->planting_tool;
                
                if(tool->seed_container == NO_ENTITY) desaturation = 1;

                if(e->type == ENTITY_ITEM) {
                    update_entity_item(e, world_t);
                    Nugget_Type nugget_type;
                    if(holds_nugget(e, &nugget_type) && nugget_type == NUGGET_SEEDS) {
                        if(tool->seed_container == NO_ENTITY || tool->seed_container == e->id)
                            desaturation = -.5f;
                    }
                }
            }
            //--
            
            draw_entity(e, world_t, gfx, room, current_user(client), hovered, false, desaturation, true);
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


    if(tweak_bool(TWEAK_SHOW_WORLD_GRID))
    {
        _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
        v4 color = { 0, 0, 0, 1 };
        float w  = .025;

        float z = .01;

        v3 p0 = { 0, 1, z };
        v3 p1 = { room_size_x, 1, z };
        float y1 = (float)room_size_y;
        while(p0.y < y1) {
            draw_line(p0, p1, V3_Z, w, color, gfx);
            p0.y += 1;
            p1.y += 1;
        }

        p0 = { 1, 0, z };
        p1 = { 1, room_size_y, z };
        float x1 = (float)room_size_x;
        while(p0.x < x1) {
            draw_line(p0, p1, V3_Z, w, color, gfx);
            p0.x += 1;
            p1.x += 1;
        }
    }

    
}
