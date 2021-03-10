
const int MAX_SUPPORT_POINTS = 16*16;


v3 tp_from_p(v3 p) {
    return { floorf(p.x), floorf(p.y), floorf(p.z) };
}

v3s tile_from_p(v3 p) {
    return { (s32)floorf(p.x), (s32)floorf(p.y), (s32)floorf(p.z) };
}


s32 tile_index_from_tile(v3s tile)
{
    return tile.y * room_size_x + tile.x;
}


s32 tile_index_from_p(v3 p)
{
    return tile_index_from_tile(tile_from_p(p));
}

v3 tp_from_index(s32 tile_index)
{
    int y = tile_index / room_size_x;
    int x = tile_index % room_size_x;
    return { (float)x, (float)y, 0 };
}


Liquid_Type liquid_type_of_container(Liquid_Container *lc)
{
    return (lc->amount == 0) ? LQ_NONE_OR_NUM : lc->liquid.type;
}

bool can_blend(Liquid_Container *a, Liquid_Container *b)
{
    auto a_type = liquid_type_of_container(a);
    auto b_type = liquid_type_of_container(b);

    if(a_type == LQ_NONE_OR_NUM || b_type == LQ_NONE_OR_NUM) return true;
    if(a_type == b_type) return true;

    return false;
}

Liquid_Fraction liquid_fraction_lerp(Liquid_Fraction a, Liquid_Fraction b, float t)
{
    float f = lerp((float)a, (float)b, t);
    return floorf(f);
}

Liquid liquid_lerp(Liquid *a, Liquid *b, float t)
{
    Liquid result = *a;
    
    Assert(a->type == b->type);
    if(a->type == LQ_NONE_OR_NUM) return *a;
    
    if(a->type == LQ_YEAST_WATER) {
        auto &yw_a = a->yeast_water;
        auto &yw_b = b->yeast_water;
        auto &yw_r = result.yeast_water;
        
        yw_r.yeast     = liquid_fraction_lerp(yw_a.yeast,     yw_b.yeast, t);
        yw_r.nutrition = liquid_fraction_lerp(yw_a.nutrition, yw_b.nutrition, t);
    }

    return result;
}

Liquid_Container blend(Liquid_Container *a, Liquid_Container *b)
{
    Assert(can_blend(a, b));

    auto a_type = liquid_type_of_container(a);
    auto b_type = liquid_type_of_container(b);

    if(a_type == LQ_NONE_OR_NUM && b_type == LQ_NONE_OR_NUM) return *a;

    Liquid_Container result;
    Zero(result);
    
    result.amount = a->amount + b->amount;

    if(a_type == LQ_NONE_OR_NUM)      result.liquid = b->liquid;
    else if(b_type == LQ_NONE_OR_NUM) result.liquid = a->liquid;
    else {
        Assert(b->amount > 0); // Because b_type != LQ_NONE_OR_NUM.
        float t = (result.amount > 0) ? (b->amount / (float)result.amount) : 0;
        result.liquid = liquid_lerp(&a->liquid, &b->liquid, t);
    }
    
    return result;
}

// NOTE *_continuous_amount is 10 times smaller than Liquid_Container.amount.
Liquid_Container liquid_container_lerp(Liquid_Container *a, Liquid_Container *b, float t, float *_continuous_amount = NULL /* @Jai: #bake */)
{
    Assert(a->liquid.type == b->liquid.type ||
           liquid_type_of_container(a) == LQ_NONE_OR_NUM ||
           liquid_type_of_container(b) == LQ_NONE_OR_NUM);

    auto a_type = liquid_type_of_container(a);
    auto b_type = liquid_type_of_container(b);

    if (a_type == LQ_NONE_OR_NUM && b_type == LQ_NONE_OR_NUM) {
        if(_continuous_amount) *_continuous_amount = 0;
        return *a;
    }
    
    auto type = a_type;
    if(type == LQ_NONE_OR_NUM) type = b_type;
    
    Liquid_Container result = *a;
    result.liquid.type = type;
    result.liquid = liquid_lerp(&result.liquid, &b->liquid, t);
    
    float amt = lerp(0.1f * result.amount, 0.1f * b->amount, t);
    if(_continuous_amount) *_continuous_amount = amt;
    
    result.amount = floorf(amt * 10.0f);

    return result;
}

Liquid_Container liquid_container_lerp(Liquid_Container *lc0, Liquid_Container *lc1, double t0, double t1, double t, float *_continuous_amount = NULL)
{
    double dur = t1 - t0;
    double t_norm = (dur <= 0) ? 1 : clamp((t - t0) / dur);
    return liquid_container_lerp(lc0, lc1, t_norm, _continuous_amount);
}

Liquid_Amount liquid_container_capacity(Item *item)
{
    Item_Type *item_type = item_types + item->type;
    
#if DEBUG
    Assert(item_type->flags & ITEM_IS_LQ_CONTAINER);
#endif

    auto vol = item_type->volume;

    switch(item->type) {

        default: return vol.x * vol.y * vol.z * 10;
    }

    Assert(false);
    return 0;
}


void simulate_liquid_properties(Liquid_Container *lc, double dt)
{
    Assert(dt >= 0);
    
    auto lq_type = liquid_type_of_container(lc);
    if(lq_type == LQ_YEAST_WATER) {
        auto &yw = lc->liquid.yeast_water;

        const Liquid_Fraction max_yeast = 800;

        // @Norelease: We should have these speeds as integers, and make sure that, for example,
        //             yeast does not increase without nutrition decreasing.
        //             Also, if one yeast grow == 2 nutrition eat, and there is only 1 nutrition left,
        //             the yeast should not be able to grow...
        const double yeast_grow_speed = 1; // @Norelease: This should be much slower. We have it like this for testing.
        const double nutrition_consume_speed = 0.5;

        double max_yeast_dt     = (max_yeast - yw.yeast) / yeast_grow_speed;
        double max_nutrition_dt = yw.nutrition           / nutrition_consume_speed;
            
        dt = min(max_yeast_dt, min(max_nutrition_dt, dt));

        yw.yeast     += roundf(dt * yeast_grow_speed);
        yw.nutrition -= roundf(dt * nutrition_consume_speed);
    } 
}

void update_entity_item(S__Entity *e, double world_t)
{
    Assert(e->type == ENTITY_ITEM);

    auto *item = &e->item_e.item;
    switch(item->type) {
        case ITEM_PLANT: {
            auto *plant = &item->plant;
            auto *state = &e->item_e.plant;

            float grow_speed = 1.0f / 60.0f;
            plant->grow_progress = state->grow_progress_on_plant + (world_t - state->t_on_plant) * grow_speed;
            plant->grow_progress = clamp(plant->grow_progress);
        } break;
    }

    auto *type = &item_types[item->type];
    if(type->flags & ITEM_IS_LQ_CONTAINER) {
        item->liquid_container = liquid_container_lerp(&e->item_e.lc0, &e->item_e.lc1, e->item_e.lc_t0, e->item_e.lc_t1, world_t);

        // Continue simulating properties after lerp t1
        double extra_dt = world_t - e->item_e.lc_t1;
        if(extra_dt > 0) {
            simulate_liquid_properties(&item->liquid_container, extra_dt);
        }
    }
}


v3 volume_p_from_tp(v3 tp, v3s volume, Quat q)
{
    v3 p = tp;
    
    if(volume.x % 2 != 0) p += rotate_vector(V3_X, q) * 0.5f;
    if(volume.y % 2 != 0) p += rotate_vector(V3_Y, q) * 0.5f;

    return p;
}

v3 item_entity_p_from_tp(v3 tp, Item *item, Quat q)
{
    return volume_p_from_tp(tp, item_types[item->type].volume, q);
}

v3 volume_tp_from_p(v3 p, v3s volume)
{
    v3 tp = p;
    
    if(volume.x % 2 != 0) p.x -= 0.5f;
    if(volume.y % 2 != 0) p.y -= 0.5f;

    return tp;
}

v3 item_entity_tp_from_p(v3 p, Item *item)
{
    return volume_tp_from_p(p, item_types[item->type].volume);
}



Player_Action make_player_entity_action(Entity_Action *action, Entity_ID target)
{
    Assert(target != NO_ENTITY);
    
    Player_Action player_action = {0};
    player_action.type = PLAYER_ACT_ENTITY;

    player_action.entity.action = *action;
    player_action.entity.target =  target;

    return player_action;
}

S__Entity create_item_entity(Item *item, v3 p, Quat q, double world_t)
{
    S__Entity e = {0};
    e.type = ENTITY_ITEM;
    e.item_e.item = *item;
    
    e.item_e.p = p;
    e.item_e.q = q;

    switch(item->type) {
        case ITEM_PLANT: {
            auto *plant_e = &e.item_e.plant;
            plant_e->t_on_plant = world_t;
            plant_e->grow_progress_on_plant = item->plant.grow_progress;
        } break;

        case ITEM_CHESS_BOARD: {
            auto *board = &e.item_e.chess_board;
            reset_chess_board(board);
        } break;
    }

    if(item_types[item->type].flags & ITEM_IS_LQ_CONTAINER) {
        e.item_e.lc0 = item->liquid_container;
        e.item_e.lc1 = item->liquid_container;
        e.item_e.lc_t0 = world_t;
        e.item_e.lc_t1 = world_t;
    }

    return e;
}

template<typename ENTITY>
void get_entity_transform(ENTITY *e, double world_t, Room *room, v3 *_p, Quat *_q)
{
    ENTITY *holder = NULL;
    if(e->held_by != NO_ENTITY)
    {
        holder = find_entity(e->held_by, room);
    }

    if(holder)
    {
        v3 holder_p;
        Quat holder_q;
        get_entity_transform(holder, world_t, room, &holder_p, &holder_q);
        *_q = holder_q;
        *_p = holder_p + V3_Z * 6;
        return;
    }
    else
    {
        
        switch(e->type) {
            case ENTITY_ITEM: {
                *_p = e->item_e.p;
                *_q = e->item_e.q;                
                return;
            } break;
            
            case ENTITY_PLAYER:
            {
                Assert(e->player_e.walk_path_length >= 2);

                auto *player_e = &e->player_e;

                if(player_e->sitting_on != NO_ENTITY) {
                    auto *sittee = find_entity(player_e->sitting_on, room);
                    if(sittee) {
                        get_entity_transform(sittee, world_t, room, _p, _q);
                        return;
                    }
                }

                // Find current path section
                double tt = e->player_e.walk_t0;
                for(int i = 1; i < player_e->walk_path_length; i++) {

                    v3 p0 = player_e->walk_path[i-1];
                    v3 p1 = player_e->walk_path[i];

                    v3     diff = p1 - p0;
                    double dist = magnitude(diff);
                    double dur  = dist / player_walk_speed;

                    double t0 = tt;
                    double t1 = t0 + dur;
                    if(world_t < t1) {

                        *_q = axis_rotation(V3_Z, atan2(diff.y, diff.x));
                    
                        if(is_zero(dur)) {
                            *_p = p1;
                        }
                        else {
                            double x = (world_t - t0) / dur;
                            *_p = lerp(p0, p1, clamp(x));
                        }
                    
                        return;
                    }

                    tt = t1;
                }

                v3 p0 = player_e->walk_path[player_e->walk_path_length-2];
                v3 p1 = player_e->walk_path[player_e->walk_path_length-1];
                v3 diff = p1 - p0;
            
                *_q = axis_rotation(V3_Z, atan2(diff.y, diff.x));
                *_p = player_e->walk_path[player_e->walk_path_length-1];
                return;
            
            } break;

            default: Assert(false); break;
        }
    }
    
    Assert(false);
    *_p = V3_ZERO;
    *_q = Q_IDENTITY;
}

template<typename ENTITY>
v3 entity_position(ENTITY *e, double world_t, Room *room)
{
    v3 p;
    Quat q; // @Unused
    get_entity_transform(e, world_t, room, &p, &q);
    return p;
}


// NOTE: hands_zoffs is the z offset for the performer entity's hands from their origin.
template<typename ENTITY>
Array<v3s, ALLOC_TMP> entity_action_positions(ENTITY *e, Entity_Action *action, float hands_zoffs, double world_t, Room *room)
{
    Array<v3s, ALLOC_TMP> positions = {0};
    
    v3 p = entity_position(e, world_t, room);
    
    if(e->type != ENTITY_ITEM) {
        Assert(false);
        array_add(positions, tile_from_p(p));
        return positions;
    }
    auto *item_e = &e->item_e;
    auto *item   = &item_e->item;

    v3s volume = item_types[item->type].volume;

    v3 forward = rotate_vector(V3_X, item_e->q);
    v3 left    = rotate_vector(V3_Y, item_e->q);
    v3 up      = rotate_vector(V3_Z, item_e->q);

    // @Cleanup: action should not be optional. We have it here for PUT_DOWN.
    if(action) {
        
        switch(action->type) {

            // @Norelease: Do the same for PUT_DOWN as we do for PICK_UP. 
            case ENTITY_ACT_PICK_UP: {
                
                v3 tp0 = p;
                tp0 -= forward * volume.x * 0.5f;
                tp0 -= left    * volume.y * 0.5f;
                
                for(auto z = -player_entity_hands_zoffs - 2; z <= 0; z++) {
                    for(int y = 0; y <= volume.y; y++) {
                        for(int x = 0; x <= volume.x; x++) {

                            // @Speed: Continuing on most squares for big volumes.
                            if(x > 0 && x < volume.x && y > 0 && y < volume.y) continue;

                            const float offs = 1.0f;
                            
                            v3 pp = tp0 + (forward * x) + (left * y) + (up * z) + V3_ONE * 0.5f; // Rounding (flooring) errors seem to happen sometimes if we don't add half a tile.
                            
                            if     (y == 0)        array_add(positions, tile_from_p(pp - left * offs));
                            else if(y == volume.y) array_add(positions, tile_from_p(pp + left * offs));
                        
                            if     (x == 0)        array_add(positions, tile_from_p(pp - forward * offs));
                            else if(x == volume.x) array_add(positions, tile_from_p(pp + forward * offs));
                            
                        }
                    }
                }

            } break;

            case ENTITY_ACT_SIT_OR_UNSIT: {
                array_add(positions, tile_from_p(p + forward * (volume.x * 0.5f + 1)));
                array_add(positions, tile_from_p(p + left    * (volume.y * 0.5f + 1)));
                array_add(positions, tile_from_p(p - left    * (volume.y * 0.5f + 1)));
                array_add(positions, tile_from_p(p - forward * (volume.x * 0.5f + 1)));
            } break;

            case ENTITY_ACT_CHESS: {
                auto *chess_action = &action->chess;
            
                Assert(item->type == ITEM_CHESS_BOARD);
                auto *board = &item_e->chess_board;
                
                v3 white_p = p + left * (volume.y * 0.5f + 1);
                v3 black_p = p - left * (volume.y * 0.5f + 1);

                white_p.z -= hands_zoffs;
                black_p.z -= hands_zoffs;

                switch(chess_action->type)
                {
                    case CHESS_ACT_MOVE: {       
                        auto *move = &chess_action->move;
                    
                        Chess_Piece piece;
                        bool found = get_chess_piece_at(move->from, board, &piece);
                    
                        if(!found) { Assert(false); break; }
                    
                        if(piece.is_black) array_add(positions, tile_from_p(black_p));
                        else               array_add(positions, tile_from_p(white_p));
                        
                    } break;

                    case CHESS_ACT_JOIN: {
                        auto *join = &chess_action->join;
                        
                        // @Norelease: We must check if there is already a player on its way to join!
                        
                        if     (join->as_black) array_add(positions, tile_from_p(black_p)); // Join as black
                        else                    array_add(positions, tile_from_p(white_p)); // Join as white
                        
                    } break;
                }
            } break;
        }
    }
        
    if(positions.n == 0) {
        array_add(positions, tile_from_p(p + forward * (volume.x * 0.5f + 1)));
    }
    
    return positions;
}


template<typename ENTITY>
AABB entity_aabb(ENTITY *e, v3 p, Quat q)
{
    AABB bbox = {0};

    float zz = atan2(2.0 * (q.w * q.z + q.x * q.y),
                     1.0 - 2.0 * (q.y * q.y + q.z * q.z));

    int quadrant = round(zz / (.25 * TAU));
    bool do_rotate_90_deg = (abs(quadrant % 2) == 1);
    
    switch(e->type) {
        case ENTITY_ITEM: {
            auto *item_e = &e->item_e;
            Assert(item_e->item.type != ITEM_NONE_OR_NUM);
    
            auto *type = &item_types[item_e->item.type];
    
            bbox.s = { (float)type->volume.x, (float)type->volume.y, (float)type->volume.z };
        } break;

        case ENTITY_PLAYER: {
            bbox.s = V3(player_entity_volume);

            // @Hack for raycasting
            bbox.s *= 0.8f;
        } break;

        default: Assert(false); break;
    }
    
    if(do_rotate_90_deg) {
        swap(&bbox.s.x, &bbox.s.y);
    }
    
    bbox.p.xy = p.xy - bbox.s.xy / 2.0f;
    bbox.p.z  = p.z;


    return bbox;
}

template<typename ENTITY>
AABB entity_aabb(ENTITY *e, double world_t, Room *room)
{
    v3 p;
    Quat q;
    get_entity_transform(e, world_t, room, &p, &q);

    return entity_aabb(e, p, q);
}

Static_Array<Surface, 8> item_entity_surfaces(S__Entity *e, double world_t, Room *room)
{
    Static_Array<Surface, 8> surfaces = {0};
    
    if(e->type != ENTITY_ITEM) return surfaces;

    
    v3 p;
    Quat q;
    get_entity_transform(e, world_t, room, &p, &q);
    Item *item = &e->item_e.item;

    v2 forward = rotate_vector(V3_X, q).xy;
    v2 left    = rotate_vector(V3_Y, q).xy;
    
    switch(item->type) {
        case ITEM_TABLE: {
            v3s vol = item_types[item->type].volume;
            
            Surface surf;            
            surf.p = p;
            surf.p.xy -= forward * vol.x * 0.5f;
            surf.p.xy -= left    * vol.y * 0.5f;
            surf.p.z  += vol.z;

            surf.s = forward * vol.x + left * vol.y;
            
            array_add(surfaces, surf);
            
        } break;

        case ITEM_BLENDER: {
            v3s vol = item_types[item->type].volume;

            // ingredients
            {
                Surface surf;
                surf.p = { p.x - vol.x * 0.5f, p.y - vol.y * 0.5f, p.z + vol.z };
                surf.s = { 1.0f, 1.0f };

                float x0 = surf.p.x;
                for(int y = 0; y < vol.y; y++) {
                    for(int x = 0; x < vol.x; x++) {
                        array_add(surfaces, surf);
                        surf.p.x += 1;
                    }
                    surf.p.x = x0;
                    surf.p.y += 1;
                }
            }

            // container
            {
                Surface surf;
                surf.p = p + V3(-vol.x * 0.5f, -vol.y * 0.5f, .2);
                surf.s = { (float)vol.x, (float)vol.y };
                array_add(surfaces, surf);
            }
                    
        } break;
    }

    // Make sure sizes are positive
    for(int i = 0; i < surfaces.n; i++) {
        auto &surf = surfaces[i];
        
        if(surf.s.x < 0) {
            surf.p.x += surf.s.x;
            surf.s.x *= -1;
        }
    
        if(surf.s.y < 0) {
            surf.p.y += surf.s.y;
            surf.s.y *= -1;
        }
    }
    //--

    return surfaces;
}



Entity *find_entity(Entity_ID id, Room *room)
{
    for(int i = 0; i < Num_Entities(room); i++) {
        auto *e = &Entities(room)[i];
        if(!Entity_Exists(e)) continue; // @Jai: Iterator for entities that always checks this.
        if(e->id == id) {
            return e;
        }
    }

    return NULL;
}

Entity *find_player_entity(User_ID user_id, Room *room)
{
    for(int i = 0; i < Num_Entities(room); i++) {
        auto *e = &Entities(room)[i];
        if(!Entity_Exists(e)) continue;
        if(e->type != ENTITY_PLAYER) continue;
        if(e->player_e.user_id == user_id) {
            return e;
        }
    }

    return NULL;
}


// @Speed @Speed @Speed @Norelease
Entity *item_entity_of_type_at(Item_Type_ID type, v3 p, double world_t, Room *room)
{
    for(int i = 0; i < Num_Entities(room); i++) {
        auto *e = Entities(room) + i;
        if(!Entity_Exists(e)) continue;
        if(e->type != ENTITY_ITEM) continue;
        if(e->item_e.item.type != type) continue;

        if(entity_position(e, world_t, room) == p) {
            return e;
        }
    }
    return NULL;
}

// @Speed @Speed @Speed @Norelease
Entity *entity_needing_support_intersecting(v3 p, double world_t, Room *room)
{
    for(int i = 0; i < Num_Entities(room); i++) {
        auto *e = Entities(room) + i;
        if(!Entity_Exists(e)) continue;

        AABB bbox = entity_aabb(e, world_t, room);
        if(aabb_contains_point(bbox, p)) return e;
    }
    return NULL;
}




bool is_supported_by(v3 *support_points, int num_support_points, Entity *potential_supporter, double world_t, Room *room, bool *support_point_satisfied_array = NULL)
{
    auto surfaces = item_entity_surfaces(potential_supporter, world_t, room);

    bool supported = false;
    
    for(int i = 0; i < surfaces.n; i++) {
        auto &surf = surfaces[i];
        for(int i = 0; i < num_support_points; i++)
        {
            auto &sp = support_points[i];

            if(!floats_equal(sp.z, surf.p.z)) continue; // @Speed: We might be able to skip this surface entirely here, if all support points have the same z.

            Rect rect = { surf.p.xy, surf.s };
            if(point_inside_rect(sp.xy, rect)) {
                if(support_point_satisfied_array) {
                    support_point_satisfied_array[i] = true;
                }
                supported = true;
            }
        }
    }

    return supported;
}

void get_support_points(AABB bbox, Static_Array<v3, MAX_SUPPORT_POINTS> *_points)
{
    _points->n = 0;
    
    float yy = bbox.p.y + 0.5f;
    while(yy < bbox.p.y + bbox.s.y) {
            
        float xx = bbox.p.x + 0.5f;
        while(xx < bbox.p.x + bbox.s.x) {
            array_add(*_points, { xx, yy, bbox.p.z });
            xx += 1;
        }
            
        yy += 1;
    }
}

// NOTE: *_supported should be in a valid state when passed! We set .n to zero, though.
template<Allocator_ID A>
void find_supported_entities(Entity *e, Room *room, Array<Entity *, A> *_supported)
{
    _supported->n = 0;
    
    auto surfaces = item_entity_surfaces(e, room->t, room);
    for(int i = 0; i < surfaces.n; i++) {
        auto &surf = surfaces[i];

        float x0 = surf.p.x + 0.5f;
        v3 pp = { x0, surf.p.y + 0.5f, surf.p.z + 0.5f};
        float x1 = pp.x + surf.s.x;
        float y1 = pp.y + surf.s.y;
        while(pp.y < y1) {
            while(pp.x < x1) {
                Entity *supported_entity = entity_needing_support_intersecting(pp, room->t, room);
                if(supported_entity && supported_entity != e) {
                    ensure_in_array(*_supported, supported_entity);
                }
                pp.x += 1;
            }
            pp.x = x0;
            pp.y += 1;
        }
    }
}


// @Speed!
Static_Array<Entity *, MAX_SUPPORT_POINTS> find_supporters(Entity *e, double world_t, Room *room)
{
    Static_Array<Entity *, MAX_SUPPORT_POINTS> supporters = {0};

    AABB bbox = entity_aabb(e, world_t, room);
    
    Static_Array<v3, MAX_SUPPORT_POINTS> support_points = {0};
    get_support_points(bbox, &support_points);
    
    for(int i = 0; i < Num_Entities(room); i++) {
        auto *potential_supporter = &Entities(room)[i];
        if(!Entity_Exists(potential_supporter)) continue;
        
        if(is_supported_by(support_points.e, support_points.n, potential_supporter, world_t, room)) {
            array_add(supporters, potential_supporter);
        }
    }

    return supporters;
}


bool item_entity_can_be_at(S__Entity *my_entity, v3 p, Quat q, double world_t, Room *room, Static_Array<Entity *, MAX_SUPPORT_POINTS> *_supporters = NULL)
{
    Assert(my_entity->type == ENTITY_ITEM);

    Static_Array<Entity *, MAX_SUPPORT_POINTS> supporters = { 0 };
    defer(if(_supporters) *_supporters = supporters;);
    
    if(_supporters) {
        Zero(*_supporters);
    }
    
    S__Entity copy = *my_entity;
    copy.item_e.p = p;
    copy.held_by = NO_ENTITY;

    AABB my_bbox = entity_aabb(&copy, world_t, room);

    bool supported_by_floor = (p.z <= 0);
    
    // FIND SUPPORT POINTS //
    Static_Array<v3, MAX_SUPPORT_POINTS> support_points = {0}; // @Speed: If we would want bigger items than this, we would probably need to do the check in some other way, anyway.
    bool support_point_satisfied[MAX_SUPPORT_POINTS] = {0};
    if(!supported_by_floor) { // NOTE: If this is false, we will end up with zero support points, which means 100% of them are satisfied.
        get_support_points(my_bbox, &support_points);
    }
    // // //
    
    for(int i = 0; i < Num_Entities(room); i++)
    {
        auto *e = &Entities(room)[i];
        if(!Entity_Exists(e)) continue;
        
        if(e->id == my_entity->id) continue;
        
        AABB other_bbox = entity_aabb(e, world_t, room);

        if(aabb_intersects_aabb(my_bbox, other_bbox))
            return false;
        
        // @Speed: We look up the entity's position, bbox etc both in is_supported_by and entity_aabb.  -EH, 2021-03-02

        if(in_array(supporters, e)) continue;

        // Are we supported by this entity?
        if(is_supported_by(support_points.e, support_points.n, e, world_t, room, support_point_satisfied)) {
            array_add(supporters, e);
        }
        
    }

    // CHECK IF WE ARE FULLY SUPPORTED //
    for(int i = 0; i < support_points.n; i++) {
        if(!support_point_satisfied[i]) return false;
    }
    // // //

    return true;
}

bool item_entity_can_be_at_tp(Entity *my_entity, v3 tp, Quat q, double world_t, Room *room, Static_Array<Entity *, MAX_SUPPORT_POINTS> *_supporters = NULL)
{
    Assert(my_entity->type == ENTITY_ITEM);
    return item_entity_can_be_at(my_entity, item_entity_p_from_tp(tp, &my_entity->item_e.item, q), q, world_t, room, _supporters);
}

bool can_place_item_entity(Item *item, v3 p, Quat q, double world_t, Room *room, Static_Array<Entity *, MAX_SUPPORT_POINTS> *_supporters = NULL)
{
    S__Entity e = create_item_entity(item, p, q, world_t);
    return item_entity_can_be_at(&e, e.item_e.p, q, world_t, room, _supporters);
}

bool can_place_item_entity_at_tp(Item *item, v3 tp, Quat q, double world_t, Room *room, Static_Array<Entity *, MAX_SUPPORT_POINTS> *_supporters = NULL)
{
    return can_place_item_entity(item, item_entity_p_from_tp(tp, item, q), q, world_t, room, _supporters);
}


Player_State player_state_of(S__Entity *player, double world_t, Room *room)
{
    Player_State state = {0};
    
    Assert(player->type == ENTITY_PLAYER);
    auto *player_e = &player->player_e;

    state.entity_id = player->id;
    state.user_id = player_e->user_id;

    state.p = entity_position(player, world_t, room);
    
    if(player->holding != NO_ENTITY)
    {
        auto *e = find_entity(player->holding, room);
        if(e) {
            Assert(e->type == ENTITY_ITEM);
            auto *item_e = &e->item_e;

            state.held_item = item_e->item;
        }
    }
    else state.held_item.type = ITEM_NONE_OR_NUM;

    state.sitting_on = player_e->sitting_on;
    
    return state;
}




// NOTE: Pass NULL if calling this from for example the Room Server. See note for entity_action_predicted_possible(). -EH, 2021-02-11
template<Allocator_ID A>
AABB *find_player_put_down_volumes(S__Entity *player, double world_t, Room *room, int *_num, S__User *user)
{
    Assert(player->type == ENTITY_PLAYER);
    auto *player_e = &player->player_e;

    auto max_volumes = ARRLEN(player_e->action_queue); // IMPORTANT: Max one volume per PUT_DOWN action!
    auto *volumes = (AABB *)alloc(sizeof(AABB) * max_volumes, A);
    *_num = 0;

    Player_State state = player_state_of(player, world_t, room);
    
    int num_actions_applied_to_state = 0;
    for(int i = 0; i < player_e->action_queue_length; i++)
    {
        auto *act = &player_e->action_queue[i];

        if(act->type == PLAYER_ACT_PUT_DOWN)
        {
            auto *x = &act->put_down;
            
            // Apply actions up til before this PUT_DOWN.
            apply_actions_to_player_state(&state, player_e->action_queue + num_actions_applied_to_state, i - num_actions_applied_to_state, world_t, room, user);
            num_actions_applied_to_state = i;

            if(state.held_item.type != ITEM_NONE_OR_NUM)
            {
                Quat q = Q_IDENTITY; // @Norelease
                v3 p = item_entity_p_from_tp(x->tp, &state.held_item, q);
                S__Entity fake_entity = create_item_entity(&state.held_item, p, q, world_t);

                AABB bbox = entity_aabb(&fake_entity, world_t, room);
                volumes[(*_num)++] = bbox;
                Assert(*_num < max_volumes);
            }
        }        
    }

    return volumes;
}


bool item_entity_can_be_moved(S__Entity *e, double world_t, Room *room)
{
    Assert(e->type == ENTITY_ITEM);
            
    auto *item_e = &e->item_e;
    auto *item   = &item_e->item;
    
    if(item_e->locked_by != NO_ENTITY) return false;

    auto surfaces = item_entity_surfaces(e, world_t, room);
    for(int i = 0; i < surfaces.n; i++) {
        auto &surf = surfaces[i];
            
        float x0 = surf.p.x + 0.5f;
        v3 pp = { x0, surf.p.y + 0.5f, surf.p.z + 0.5f };
        float y1 = surf.p.y + surf.s.y;
        float x1 = surf.p.x + surf.s.x;
        
        while(pp.y < y1) {
            while(pp.x < x1) {
                Entity *sup = entity_needing_support_intersecting(pp, world_t, room);
                if(sup != NULL && sup != e) return false; // We are supporting another entity, so we can't be moved.
                pp.x += 1;
            }
            pp.x = x0;
            pp.y += 1;
        }
    }

    switch(item->type) {
        case ITEM_MACHINE: {
            auto *machine = &item_e->machine;
            if(machine->start_t > machine->stop_t) return false;
        } break;

        case ITEM_CHESS_BOARD: {
            auto *chess = &item_e->chess_board;
            if(chess->white_player.user != NO_USER) return false;
            if(chess->black_player.user != NO_USER) return false;
        } break;
    }
    
    return true;
}



// NOTE: user can be null, when we for example do this on the Room Server.
//       The room server don't know much about the user. So (IMPORTANT) it will not check
//       stuff that is in the User struct. But it should then in some way ask the User
//       Server if the action is possible, if there is a possibility that the User Server
//       would say no.
//
//       An EXAMPLE is the pick-up action. On the client, we check that we can pick up the
//       item from the room, and that we have available space in our User.inventory.
//       The Room Server checks the first part locally, but the inventory part will get
//       checked by the User Server when we do the item transaction.
//
// NOTE: @Norelease: We can't yet pass another world_t than the room's t. If we want
//                   to do that, we need to simulate the room up to that t (temporarily)
//                   before we check the state of entities!
bool entity_action_predicted_possible(Entity_Action action, S__Entity *e, Player_State *player_state, double world_t, Room *room, bool *_sitting_allowed, S__User *user = NULL)
{
    Function_Profile();
    
    Assert(e);
    Assert(player_state);
    Assert(player_state->user_id != NO_USER);

    *_sitting_allowed = true;

    // Update the entity's item so we can check its state.
    if(e->type == ENTITY_ITEM) {
        update_entity_item(e, world_t);
    }

    
    if(e->held_by != NO_ENTITY &&
       e->held_by != player_state->entity_id) return false;

    
    switch(action.type) {
        case ENTITY_ACT_PICK_UP: {
            Scoped_Profile("ENTITY_ACT_PICK_UP");
            
            if(e->type != ENTITY_ITEM) return false;

            if(player_state->held_item.type != ITEM_NONE_OR_NUM) return false;
            
            auto *item_e = &e->item_e;
            auto *item   = &e->item_e.item;
            
            if(item->owner != player_state->user_id) return false;

            if(!item_entity_can_be_moved(e, world_t, room)) return false;
            
            return true;
            
        } break;
        
        case ENTITY_ACT_PLACE_IN_INVENTORY: {

            if(e->type != ENTITY_ITEM) return false;
            
            auto *item_e = &e->item_e;
            auto *item   = &e->item_e.item;

            if(item->owner != player_state->user_id) return false;

            if(!item_entity_can_be_moved(e, world_t, room)) return false;
    
            if(user) {
                if(!inventory_has_available_space_for_item_type(item->type, user)) return false;
            }

            return true;
        } break;

        case ENTITY_ACT_HARVEST: {

            if(e->type != ENTITY_ITEM) return false;

            auto *item = &e->item_e.item;
            if(item->type != ITEM_PLANT) return false;
            
            if(item->plant.grow_progress < 0.75f) return false;

            if(player_state->held_item.type != ITEM_NONE_OR_NUM) return false;

            return true;
            
        } break;

        case ENTITY_ACT_SET_POWER_MODE: {
            auto *x = &action.set_power_mode;
            
            if(e->type != ENTITY_ITEM) return false;

            auto *item = &e->item_e.item;
            if(item->type != ITEM_MACHINE) return false;            

            auto *machine_e = &e->item_e.machine;
            if(x->set_to_on) {
                if(machine_e->start_t > machine_e->stop_t) return false; // Already started
            }
            else {
                if(machine_e->stop_t >= machine_e->start_t) return false; // Already stopped.
            }

            return true;
        };

        case ENTITY_ACT_WATER: {
            auto *x = &action.set_power_mode;
            
            if(e->type != ENTITY_ITEM) return false;

            auto *item = &e->item_e.item;
            if(item->type != ITEM_PLANT) return false;

            auto *plant_e = &e->item_e.plant;

            if(!(item_types[player_state->held_item.type].flags & ITEM_IS_LQ_CONTAINER)) return false;
            auto *lc = &player_state->held_item.liquid_container;

            if(lc->liquid.type != LQ_WATER) return false;
            if(lc->amount < 2) return false; // @Norelease @Volatile: define constant somewhere. We have it in entity_action_predicted_possible and perform_entity_action_if_possible.

            return true;
        } break;

        case ENTITY_ACT_CHESS: {
            auto *chess_action = &action.chess;

            if(e->type != ENTITY_ITEM) return false;
            if(e->item_e.item.type != ITEM_CHESS_BOARD) return false;

            if(player_state->held_item.type != ITEM_NONE_OR_NUM) return false;

            auto *board = &e->item_e.chess_board;

            return chess_action_possible(chess_action, player_state->user_id, board);
            
        } break;

        case ENTITY_ACT_SIT_OR_UNSIT: {
            auto *sit = &action.sit_or_unsit;
            
            if(e->type != ENTITY_ITEM) return false;
            
            auto *item = &e->item_e.item;
            if(item->type != ITEM_CHAIR) return false;

            if(sit->unsit  && player_state->sitting_on != e->id) return false; // Can't unsit when we don't sit.
            if(!sit->unsit && player_state->sitting_on == e->id) return false; // Can't sit when we already sit.

            *_sitting_allowed = sit->unsit; // We can't already be sitting when we do the sit action.
            
            // @Norelease: Check if someone else sits here. @Continue

            return true;
            
        } break;

        default: Assert(false); return false;
    }
    
    Assert(false);
    return true;
}


// @BadName
struct Player_Action_Prediction_Info
{
    union {
        struct {
            Static_Array<Entity *, MAX_SUPPORT_POINTS> supporters;
        } put_down;
    };
};

// NOTE: See notes for entity_action_predicted_possible().
// IMPORTANT: You can't pass _found_path_duration without passing _found_path.
bool player_action_predicted_possible(Player_Action *action, Player_State *player_state, double world_t, Room *room, Optional<Player_Action> *_action_needed_before = NULL, Array<v3, ALLOC_TMP> *_found_path = NULL, double *_found_path_duration = NULL, Player_Action_Prediction_Info *_info = NULL, S__User *user = NULL)
{
    Function_Profile();

    if(_action_needed_before) _action_needed_before->present = false;
    
    Assert(player_state);
    Assert(player_state->user_id != NO_USER);

    enum Transport_Mode {
        WALK,
        TELEPORT,
        NONE
    };

    Transport_Mode transport_needed = NONE;
    Array<v3s, ALLOC_TMP> possible_p1s = {0};

    bool sitting_allowed = true;
    
    switch(action->type) { // @Jai: #complete

        case PLAYER_ACT_ENTITY: {
            Scoped_Profile("PLAYER_ACT_ENTITY");
            
            auto *entity_act = &action->entity;

            S__Entity *e = find_entity(entity_act->target, room);
            if(!e) return false;

            if(!entity_action_predicted_possible(entity_act->action, e, player_state, world_t, room, &sitting_allowed, user)) return false;
                        
            if(e->held_by == NO_ENTITY) {
                possible_p1s = entity_action_positions(e, &entity_act->action, player_entity_hands_zoffs, world_t, room);

                if(entity_act->action.type == ENTITY_ACT_SIT_OR_UNSIT &&
                   entity_act->action.sit_or_unsit.unsit == true) {
                    transport_needed = TELEPORT;
                } else {
                    transport_needed = WALK;
                }
            }
            
        } break;

        case PLAYER_ACT_WALK: {
            Scoped_Profile("PLAYER_ACT_WALK");
            
            array_add(possible_p1s, tile_from_p(action->walk.p1));
            transport_needed = WALK;
        } break;

        case PLAYER_ACT_PUT_DOWN: {
            Scoped_Profile("PLAYER_ACT_PUT_DOWN");
            
            auto *put_down = &action->put_down;

            if(player_state->held_item.type == ITEM_NONE_OR_NUM) return false;

            Quat put_down_q = Q_IDENTITY; // @Norelease: Rotation for PUT_DOWN
            v3 put_down_p = item_entity_p_from_tp(put_down->tp, &player_state->held_item, put_down_q);
            
            S__Entity held_entity_replica = create_item_entity(&player_state->held_item, put_down_p, put_down_q, world_t);
            
            // @Norelease: Check that the put down entity won't collide with anything.

            // If we just do can_place_item_at(), these statements are true:
            // @Norelease: We will collide with ourselves here if the pick-up has not yet happened.
            // @Norelease: We will collide with other entities even if there is a pick-up of them earlier in the queue.
            // @Norelease: We will NOT collide with other entities' put-down volumes, even if their put-downs happens earlier than this one.
            //             So it is possible to put down two entities in the same spot right now.

            // This is @Temporary (See comments above).
            if(!item_entity_can_be_at(&held_entity_replica, put_down_p, put_down_q, world_t, room, &_info->put_down.supporters))
                return false;

            // @Cleanup @Hack !!!
            Entity_Action dummy_action = {0};
            dummy_action.type = ENTITY_ACT_PICK_UP;
            
            // @Cleanup: action parameter should not be optional. PUT_DOWN should work differently... Should probably be an entity action.
            possible_p1s = entity_action_positions(&held_entity_replica, &dummy_action, player_entity_hands_zoffs, world_t, room);
            transport_needed = WALK;
        } break;
            
        default: Assert(false); return false;
    };


    if(transport_needed != NONE && player_state->sitting_on != NO_ENTITY)
    {
        // Do we need to unsit first?

        v3s player_state_tp = tile_from_p(player_state->p);
        bool need_to_move = true;
        // We don't need to unsit if we're already on one
        // of the available action positions.
        for(int i = 0; i < possible_p1s.n; i++)
        {
            if(possible_p1s[i] == player_state_tp) {
                need_to_move = false;
                break;
            }
        }

        bool action_is_unsit = (action->type == PLAYER_ACT_ENTITY &&
                                action->entity.action.type == ENTITY_ACT_SIT_OR_UNSIT &&
                                action->entity.action.sit_or_unsit.unsit);

        if((need_to_move || !sitting_allowed) && !action_is_unsit) {
            // We need to unsit first.
            if(_action_needed_before)
            {
                Entity_Action e_act = {0};
                e_act.type = ENTITY_ACT_SIT_OR_UNSIT;
                e_act.sit_or_unsit.unsit = true;
            
                auto p_act = make_player_entity_action(&e_act, player_state->sitting_on);
                *_action_needed_before = p_act;
            }
        
            return false;
        }
        else if (!need_to_move) {
            transport_needed = NONE;
        }
    }
    
    switch(transport_needed)
    {
        case WALK: {
            if(possible_p1s.n == 0) return false;

            if(sitting_allowed) {

                bool no_walkable_p1s = true;
                for(int i = 0; i < possible_p1s.n; i++) {
                    v3s tile = possible_p1s[i];
                    s32 tile_ix = tile_index_from_tile(tile);
                        
                    if(!(room->walk_map.nodes[tile_ix].flags & UNWALKABLE)) {
                        no_walkable_p1s = false;
                        break;
                    }
                }

                if(no_walkable_p1s) {
                    for(int i = 0; i < possible_p1s.n; i++) {
                
                        Entity *chair = item_entity_of_type_at(ITEM_CHAIR, V3(possible_p1s[i]), world_t, room);
                        if(chair) {
                            // We can make this action possible by sitting on chair first.
                            if(_action_needed_before)
                            {
                                Entity_Action e_act = {0};
                                e_act.type = ENTITY_ACT_SIT_OR_UNSIT;
                                e_act.sit_or_unsit.unsit = false;
            
                                auto p_act = make_player_entity_action(&e_act, chair->id);
                                *_action_needed_before = p_act;
                            }
        
                            return false;
                        }
                    }
                }
            }
                
            return find_path_to_any(player_state->p, possible_p1s.e, possible_p1s.n, &room->walk_map, true, _found_path, _found_path_duration);
        } break;

        case TELEPORT: {

            if(possible_p1s.n == 0) return false;

            for(int i = 0; i < possible_p1s.n; i++)
            {
                v3s tile = possible_p1s[i];
                s32 tile_ix = tile_index_from_tile(tile);
                
                if(!(room->walk_map.nodes[tile_ix].flags & UNWALKABLE)) {

                    if(_found_path) {
                        _found_path->n = 0;
                        array_add_uninitialized(*_found_path, 2);
                        
                        (*_found_path)[0] = V3(tile);
                        (*_found_path)[1] = V3(tile);
                        
                        if(_found_path_duration) *_found_path_duration = 0;
                    }
                    return true;
                }
            }
            return false;
            
        } break;

        case NONE: {
            if (_found_path) {
                Zero(*_found_path);
                if(_found_path_duration) *_found_path_duration = 0;
            }
            
            return true;
        } break;

        default: Assert(false); return false;
    }

    
}



// Pass user = NULL if you're a Room Server.
// NOTE: If check_possible = false, we assume the caller has already checked that the actions are possible.
// NOTE: Returns true if all actions are predicted possible.
bool apply_actions_to_player_state(Player_State *state, Player_Action *actions, int num_actions, double world_t, Room *room, S__User *user)
{
    Function_Profile();

    bool all_actions_predicted_possible = true;
    
    for(int i = 0; i < num_actions; i++) {
        auto *act = &actions[i];

        bool possible = true;

        Player_Action_Prediction_Info prediction_info = {0};
        
        Array<v3, ALLOC_TMP> path = {0}; // @Speed: We should send this in, and player_action should set .n = 0. Instead of allocating new memory every time
        Optional<Player_Action> action_needed_before;
        while(!player_action_predicted_possible(act, state, world_t, room, &action_needed_before, &path, NULL, &prediction_info, user)) {

            Player_Action needed_act;
            if(get(action_needed_before, &needed_act)) {
                if(apply_actions_to_player_state(state, &needed_act, 1, world_t, room, user)) continue;
            }
        
            possible = false;
            break;
        }

        if(!possible) {
            all_actions_predicted_possible = false;
            continue;
        }

        if(path.n > 0) {
            Assert(path.n >= 2);
            state->p = path[path.n-1];
        }
        
        switch(act->type) {
            case PLAYER_ACT_ENTITY:
            {    
                auto *entity_act = &act->entity;

                auto *e = find_entity(entity_act->target, room);
                if(!e) continue;
                
                Assert(e->type == ENTITY_ITEM);
                
                switch(entity_act->action.type)
                {
                    case ENTITY_ACT_PICK_UP: {
                        state->held_item = e->item_e.item;
                    } break;

                    case ENTITY_ACT_HARVEST: {
                        Item fruit = {0};
                        fruit.type = ITEM_FRUIT;
                        fruit.owner = state->user_id;
                        state->held_item = fruit;
                    } break;

                    case ENTITY_ACT_PLACE_IN_INVENTORY: {
                        if(state->held_item.type != ITEM_NONE_OR_NUM &&
                           state->held_item.id   == e->item_e.item.id)
                        {
                            state->held_item.type = ITEM_NONE_OR_NUM;
                        }
                    } break;

                    case ENTITY_ACT_WATER: {
                        Assert(item_types[state->held_item.type].flags & ITEM_IS_LQ_CONTAINER); // player_action_predicted_possible() should have checked this.
                        auto *lc = &state->held_item.liquid_container;

                        Assert(lc->liquid.type == LQ_WATER); // player_action_predicted_possible() should have checked this.
                        Assert(lc->amount >= 2); // player_action_predicted_possible() should have checked this.
                        
                        // @Volatile: We do this in two places. Can't we do affect_held_item(action) or something?
                        lc->amount -= 2; // @Norelease @Volatile: define constant somewhere. We have it in entity_action_predicted_possible and perform_entity_action_if_possible.

                    } break;

                    case ENTITY_ACT_SIT_OR_UNSIT: {
                        auto *sit = &entity_act->action.sit_or_unsit;
                        if(sit->unsit) {
                            Assert(state->sitting_on == e->id);
                            state->sitting_on = NO_ENTITY;
                        } else {
                            Assert(state->sitting_on == NO_ENTITY);
                            state->sitting_on = e->id;
                            state->p          = entity_position(e, world_t, room); // @Speed
                        }
                    } break;
                }
            }
            break;

            case PLAYER_ACT_PUT_DOWN: {
                state->held_item.type = ITEM_NONE_OR_NUM;
            } break;
        }
        
    }

    return all_actions_predicted_possible;
}
