

v3s tile_from_p(v3 p) {
    return { (s32)roundf(p.x), (s32)roundf(p.y), (s32)roundf(p.z) };
}

s32 tile_index_from_p(v3 tp)
{
    auto tile = tile_from_p(tp);
    return tp.y * room_size_x + tp.x;
}

v3 tp_from_index(s32 tile_index)
{
    int y = tile_index / room_size_x;
    int x = tile_index % room_size_x;
    return { (float)x, (float)y, 0 };
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
}


v3 volume_p_from_tp(v3 tp, v3s volume)
{
    v3 p = tp;
    
    if(volume.x % 2 != 0) p.x += 0.5f;
    if(volume.y % 2 != 0) p.y += 0.5f;

    return p;
}

v3 item_entity_p_from_tp(v3 tp, Item *item)
{
    return volume_p_from_tp(tp, item_types[item->type].volume);
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

S__Entity create_item_entity(Item *item, v3 p, double world_t, Quat q = Q_IDENTITY)
{
    S__Entity e = {0};
    e.type = ENTITY_ITEM;
    e.item_e.item = *item;
    
    e.item_e.p = p;
    e.item_e.q = q;

    switch(e.item_e.item.type) {
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

    return e;
}

template<typename ENTITY, typename ROOM>
void get_entity_transform(ENTITY *e, double world_t, ROOM *room, v3 *_p, Quat *_q)
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

                        *_q = axis_rotation(V3_Z, atan2(diff.x, diff.y));
                    
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
            
                *_q = axis_rotation(V3_Z, atan2(diff.x, diff.y));
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

template<typename ENTITY, typename ROOM>
v3 entity_position(ENTITY *e, double world_t, ROOM *room)
{
    v3 p;
    Quat q; // @Unused
    get_entity_transform(e, world_t, room, &p, &q);
    return p;
}

template<typename ENTITY, typename ROOM>
Array<v3, ALLOC_TMP> entity_action_positions(ENTITY *e, Entity_Action *action, double world_t, ROOM *room)
{
    Array<v3, ALLOC_TMP> positions = {0};
    
    v3 p  = entity_position(e, world_t, room);
    
    if(e->type != ENTITY_ITEM) {
        Assert(false);
        array_add(positions, p);
        return positions;
    }
    auto *item_e = &e->item_e;
    auto *item   = &item_e->item;

    v3s volume = item_types[item->type].volume;

    v3 forward = rotate_vector(V3_Y, item_e->q);
    v3 right   = rotate_vector(V3_X, item_e->q);

    // @Cleanup: action should not be optional. We have it here for PUT_DOWN.
    if(action) {
        
        switch(action->type) {

            // @Norelease: Do the same for PUT_DOWN as we do for PICK_UP. 
            case ENTITY_ACT_PICK_UP: {

                v3 tp0  = p;
                tp0.xy -= volume.xy * 0.5f;
                
                for(int y = 0; y < volume.y+1; y++) {
                    for(int x = 0; x < volume.x+1; x++) {

                        v3 pp = tp0;
                        pp.x += x;
                        pp.y += y;
                        
                        // @Speed: Continuing on most squares for big volumes.
                        if(x > 0 && x < volume.x &&
                           y > 0 && y < volume.y) continue;

                        const float offs = 1.0f;
                        
                        if     (x == 0)        array_add(positions, pp - right * offs);
                        else if(x == volume.x) array_add(positions, pp + right * offs);
                        
                        if     (y == 0)        array_add(positions, pp - forward * offs);
                        else if(y == volume.y) array_add(positions, pp + forward * offs);
                    }
                }
            } break;

            case ENTITY_ACT_SIT_OR_UNSIT: {
                array_add(positions, p + forward * (volume.y * 0.5f + 1));
                array_add(positions, p - right   * (volume.x * 0.5f + 1));
                array_add(positions, p + right   * (volume.x * 0.5f + 1));
                array_add(positions, p - forward * (volume.y * 0.5f + 1));
            } break;

            case ENTITY_ACT_CHESS: {
                auto *chess_action = &action->chess;
            
                Assert(item->type == ITEM_CHESS_BOARD);
                auto *board = &item_e->chess_board;

                v3 white_p = p + forward * (volume.y * 0.5f + 1);
                v3 black_p = p - forward * (volume.y * 0.5f + 1);

                switch(chess_action->type)
                {
                    case CHESS_ACT_MOVE: {       
                        auto *move = &chess_action->move;
                    
                        Chess_Piece piece;
                        bool found = get_chess_piece_at(move->from, board, &piece);
                    
                        if(!found) { Assert(false); break; }
                    
                        if(piece.is_black) array_add(positions, black_p);
                        else               array_add(positions, white_p);
                        
                    } break;

                    case CHESS_ACT_JOIN: {
                        auto *join = &chess_action->join;
                        
                        // @Norelease: We must check if there is already a player on its way to join!
                        
                        if     (join->as_black) array_add(positions, black_p); // Join as black
                        else                    array_add(positions, white_p); // Join as white
                        
                    } break;
                }
            } break;
        }
    }
        
    if(positions.n == 0) {
        array_add(positions, p + forward * (volume.y * 0.5f + 1));
    }
    
    return positions;
}



template<typename ENTITY, typename ROOM>
AABB entity_aabb(ENTITY *e, double world_t, ROOM *room)
{
    AABB bbox = {0};
    auto p = entity_position(e, world_t, room);

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
    
    bbox.p.xy = p.xy - bbox.s.xy / 2.0f;
    bbox.p.z  = p.z;

    return bbox;
}


template<typename ENTITY, typename ROOM>
bool item_entity_can_be_at(S__Entity *my_entity, v3 p, double world_t, ENTITY *entities, s64 num_entities, ROOM *room)
{
    Assert(my_entity->type == ENTITY_ITEM);
    
    S__Entity copy = *my_entity;
    copy.item_e.p = p;
    copy.held_by = NO_ENTITY;

    AABB my_bbox = entity_aabb(&copy, world_t, room);
    
    for(int i = 0; i < num_entities; i++)
    {
        auto *e = entities + i;
        if(e->id == my_entity->id) continue;
        
        AABB other_bbox = entity_aabb(e, world_t, room);
        
        if(aabb_intersects_aabb(my_bbox, other_bbox))
            return false;
    }
    
    return true;
}

template<typename ENTITY, typename ROOM>
bool item_entity_can_be_at_tp(ENTITY *my_entity, v3 tp, double world_t, ENTITY *entities, s64 num_entities, ROOM *room)
{
    Assert(my_entity->type == ENTITY_ITEM);
    return item_entity_can_be_at(my_entity, item_entity_p_from_tp(tp, &my_entity->item_e.item), world_t, entities, num_entities, room);
}

template<typename ENTITY, typename ROOM>
bool can_place_item_entity(Item *item, v3 p, double world_t, ENTITY *entities, s64 num_entities, ROOM *room)
{
    S__Entity e = create_item_entity(item, p, world_t);
    return item_entity_can_be_at(&e, e.item_e.p, world_t, entities, num_entities, room);
}

template<typename ENTITY, typename ROOM>
bool can_place_item_entity_at_tp(Item *item, v3 tp, double world_t, ENTITY *entities, s64 num_entities, ROOM *room)
{
    return can_place_item_entity(item, item_entity_p_from_tp(tp, item), world_t, entities, num_entities, room);
}


template<typename ROOM>
Player_State player_state_of(S__Entity *player, double world_t, ROOM *room)
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


// Pass user = NULL if you're a Room Server.
// NOTE: If check_possible = false, we assume the caller has already checked that the actions are possible.
// NOTE: Returns true if all actions are predicted possible.
template<typename ROOM>
bool apply_actions_to_player_state(Player_State *state, Player_Action *actions, int num_actions, double world_t, ROOM *room, S__User *user)
{
    Function_Profile();

    bool all_actions_predicted_possible = true;
    
    for(int i = 0; i < num_actions; i++) {
        auto *act = &actions[i];

        bool possible = true;
        
        Array<v3, ALLOC_TMP> path = {0}; // @Speed: We should send this in, and player_action should set .n = 0. Instead of allocating new memory every time
        Optional<Player_Action> action_needed_before;
        while(!player_action_predicted_possible(act, state, world_t, room, &action_needed_before, &path, NULL, user)) {

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

                    case ENTITY_ACT_PLACE_IN_INVENTORY: {
                        if(state->held_item.type != ITEM_NONE_OR_NUM &&
                           state->held_item.id   == e->item_e.item.id)
                        {
                            state->held_item.type = ITEM_NONE_OR_NUM;
                        }
                    } break;

                    case ENTITY_ACT_WATER: {
                        Assert(state->held_item.type == ITEM_WATERING_CAN); // player_action_predicted_possible() should have checked this.
                        auto *can = &state->held_item.watering_can;
                        
                        // @Volatile: We do this in two places. Can't we do affect_held_item(action) or something?
                        can->water_level -= 0.25f; // @Norelease @Volatile: define constant somewhere. We have it in entity_action_predicted_possible and perform_entity_action_if_possible.

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

// NOTE: Pass NULL if calling this from for example the Room Server. See note for entity_action_predicted_possible(). -EH, 2021-02-11
template<Allocator_ID A, typename ROOM>
AABB *find_player_put_down_volumes(S__Entity *player, double world_t, ROOM *room, int *_num, S__User *user)
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
                v3 p = item_entity_p_from_tp(x->tp, &state.held_item);
                S__Entity fake_entity = create_item_entity(&state.held_item, p, world_t, Q_IDENTITY);

                AABB bbox = entity_aabb(&fake_entity, world_t, room);
                volumes[(*_num)++] = bbox;
                Assert(*_num < max_volumes);
            }
        }        
    }

    return volumes;
}


bool item_entity_can_be_moved(S__Entity *e)
{
    Assert(e->type == ENTITY_ITEM);
            
    auto *item_e = &e->item_e;
    auto *item   = &e->item_e.item;
    
    if(item->type == ITEM_MACHINE) {
        auto *machine = &item_e->machine;
        if(machine->start_t > machine->stop_t) return false;
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
bool entity_action_predicted_possible(Entity_Action action, S__Entity *e, Player_State *player_state, double world_t, bool *_sitting_allowed, S__User *user = NULL)
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

            if(!item_entity_can_be_moved(e)) return false;
            
            return true;
            
        } break;
        
        case ENTITY_ACT_PLACE_IN_INVENTORY: {

            if(e->type != ENTITY_ITEM) return false;
            
            auto *item_e = &e->item_e;
            auto *item   = &e->item_e.item;

            if(item->owner != player_state->user_id) return false;

            if(!item_entity_can_be_moved(e)) return false;
    
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

            if(user) {
                // @Temporary: @Norelease Should check if harvested item(s) fits in inventory, not if the plant itself fits.
                if(!inventory_has_available_space_for_item_type(e->item_e.item.type, user)) return false;
            }
            
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

            if(player_state->held_item.type != ITEM_WATERING_CAN) return false;

            auto *can = &player_state->held_item.watering_can;
            if(can->water_level < 0.25f) return false; // @Norelease @Volatile: define constant somewhere. We have it in entity_action_predicted_possible and perform_entity_action_if_possible.

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

// NOTE: See notes for entity_action_predicted_possible().
// IMPORTANT: You can't pass _found_path_duration without passing _found_path.
template<typename ROOM>
bool player_action_predicted_possible(Player_Action *action, Player_State *player_state, double world_t, ROOM *room, Optional<Player_Action> *_action_needed_before = NULL, Array<v3, ALLOC_TMP> *_found_path = NULL, double *_found_path_duration = NULL, S__User *user = NULL)
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
    Array<v3, ALLOC_TMP> possible_p1s = {0};

    bool sitting_allowed = true;
    
    switch(action->type) { // @Jai: #complete

        case PLAYER_ACT_ENTITY: {
            Scoped_Profile("PLAYER_ACT_ENTITY");
            
            auto *entity_act = &action->entity;

            S__Entity *e = find_entity(entity_act->target, room);
            if(!e) return false;

            if(!entity_action_predicted_possible(entity_act->action, e, player_state, world_t, &sitting_allowed, user)) return false;
                        
            if(e->held_by == NO_ENTITY) {
                possible_p1s = entity_action_positions(e, &entity_act->action, world_t, room);

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
            
            array_add(possible_p1s, action->walk.p1);
            transport_needed = WALK;
        } break;

        case PLAYER_ACT_PUT_DOWN: {
            Scoped_Profile("PLAYER_ACT_PUT_DOWN");
            
            auto *put_down = &action->put_down;

            if(player_state->held_item.type == ITEM_NONE_OR_NUM) return false;

            v3 put_down_p = item_entity_p_from_tp(put_down->tp, &player_state->held_item);
            
            S__Entity held_entity_replica = create_item_entity(&player_state->held_item, put_down_p, world_t, Q_IDENTITY /* @Norelease: Have q in PUT_DOWN action */);
            
            // @Norelease: Check that the put down entity won't collide with anything.

            // If we just do can_place_item_at(), these statements are true:
            // @Norelease: We will collide with ourselves here if the pick-up has not yet happened.
            // @Norelease: We will collide with other entities even if there is a pick-up of them earlier in the queue.
            // @Norelease: We will NOT collide with other entities' put-down volumes, even if their put-downs happens earlier than this one.
            //             So it is possible to put down two entities in the same spot right now.

            // This is @Temporary (See comments above).


            // @Cleanup @Hack !!!
            Entity_Action dummy_action = {0};
            dummy_action.type = ENTITY_ACT_PICK_UP;
            
            // @Cleanup: action parameter should not be optional. PUT_DOWN should work differently... Should probably be an entity action.
            possible_p1s = entity_action_positions(&held_entity_replica, &dummy_action, world_t, room);
            transport_needed = WALK;
        } break;
            
        default: Assert(false); return false;
    };


    if(transport_needed != NONE && player_state->sitting_on != NO_ENTITY)
    {
        // Do we need to unsit first?

        bool need_to_move = true;
        // We don't need to unsit if we're already on one
        // of the available action positions.
        for(int i = 0; i < possible_p1s.n; i++)
        {
            if(possible_p1s[i] == player_state->p) {
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
    }
    
    switch(transport_needed)
    {
        case WALK: {
            if(possible_p1s.n == 0) return false;
            return find_path_to_any(player_state->p, possible_p1s.e, possible_p1s.n, &room->walk_map, true, _found_path, _found_path_duration);
        } break;

        case TELEPORT: {

            if(possible_p1s.n == 0) return false;

            for(int i = 0; i < possible_p1s.n; i++)
            {
                v3 p = possible_p1s[i];
                s32 tile_ix = tile_index_from_p(p);
                
                if(!(room->walk_map.nodes[tile_ix].flags & UNWALKABLE)) {

                    if(_found_path) {
                        _found_path->n = 0;
                        array_add_uninitialized(*_found_path, 2);
                        
                        (*_found_path)[0] = p;
                        (*_found_path)[1] = p;
                        
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
