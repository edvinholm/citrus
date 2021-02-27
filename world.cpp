
// INVENTORY //

Item *get_selected_inventory_item(User *user, bool accept_reserved = false)
{
    if(user->selected_inventory_item_plus_one == 0) return NULL;

    Inventory_Slot *slot = &user->inventory[user->selected_inventory_item_plus_one-1];
    if(!(slot->flags & INV_SLOT_FILLED)) return NULL;
    if(!accept_reserved && (slot->flags & INV_SLOT_RESERVED)) return NULL;

    Assert(slot->item.type != ITEM_NONE_OR_NUM);
    return &slot->item;
}

void inventory_deselect(User *user)
{
    user->selected_inventory_item_plus_one = 0;
}

void empty_inventory_slot_locally(int slot_ix, User *user)
{
    Assert(slot_ix >= 0);
    Assert(slot_ix < ARRLEN(user->inventory));

    auto *slot = &user->inventory[slot_ix];

    Assert(!(slot->flags & INV_SLOT_RESERVED)); // Right now, we have no situation when we would empty a reserved slot locally. -EH, 2021-02-04
    
    slot->flags &= ~(INV_SLOT_FILLED);
}

void inventory_remove_item_locally(Item_ID id, User *user)
{
    for(int i = 0; i < ARRLEN(user->inventory); i++)
    {
        auto *slot = &user->inventory[i];
        if(!(slot->flags & INV_SLOT_FILLED)) continue;
        
        if(slot->item.id == id) {
            empty_inventory_slot_locally(i, user);
            return;
        }
    }

    Assert(false);
}


bool select_next_inventory_item_of_type(Item_Type_ID type, User *user)
{
    auto selected_slot = user->selected_inventory_item_plus_one-1;
    auto slot_at = selected_slot + 1;
    auto *inventory = user->inventory;
    
    while(true)
    {
        if(slot_at == selected_slot) break;
        if(slot_at >= ARRLEN(user->inventory)) slot_at = 0;

        auto *slot = &inventory[slot_at];
        if((slot->flags & INV_SLOT_FILLED) && !(slot->flags & INV_SLOT_RESERVED))
        {
            if(slot->item.type == type) {
                user->selected_inventory_item_plus_one = slot_at + 1;
                return true;
            }
        }
        
        slot_at++;
    }

    return false;
}

// //////// //



// WORLD //

double world_time_for_room(Room *room, double system_time)
{
    return system_time + room->time_offset;
}

Entity *raycast_against_entities(Ray ray, Room *room, double world_t, v3 *_hit_p = NULL)
{
    Entity *closest_hit   = NULL;
    float   closest_ray_t = FLT_MAX;
    
    for(int i = 0; i < room->entities.n; i++)
    {
        auto *e = &room->entities[i];

        AABB bbox = entity_aabb(e, world_t, room);
           
        float ray_t;
        v3 intersection;
        if(ray_intersects_aabb(ray, bbox, &intersection, &ray_t)) {
            if(ray_t < closest_ray_t) {
                closest_hit = e;
                closest_ray_t = ray_t;
                    
                if(_hit_p) *_hit_p = intersection;
            }
        }
    }

    return closest_hit;
}

bool raycast_against_floor(Ray ray, v3 *_hit)
{
    if(is_zero(ray.dir.z)) return false;
    
    *_hit = ray.p0 + ray.dir * (-ray.p0.z / ray.dir.z);
    return true;
}

m4x4 world_projection_matrix(Rect viewport, float z_offset/* = 0*/)
{
    float x_mul, y_mul;
    if(viewport.w < viewport.h) {
        x_mul = 1;
        y_mul = 1.0f/(viewport.h / max(0.0001f, viewport.w));
    }
    else {
        y_mul = 1;
        x_mul = 1.0f/(viewport.w / max(0.0001f, viewport.h));
    }

    // TODO @Speed: @Cleanup: Combine matrices

    m4x4 world_projection = make_m4x4(
        x_mul, 0, 0, 0,
        0, y_mul, 0, 0,
        0, 0, -0.01, z_offset,
        0, 0, 0, 1);

    m4x4 rotation = rotation_matrix(axis_rotation(V3_X, TAU * 0.125));
    world_projection = matmul(rotation, world_projection);

    rotation = rotation_matrix(axis_rotation(V3_Z, TAU * -0.125));
    world_projection = matmul(rotation, world_projection);

    float diagonal_length = sqrt(room_size_x * room_size_x + room_size_y * room_size_y);
   
    m4x4 scale = scale_matrix(V3_ONE * (2.0 / diagonal_length));
    world_projection = matmul(scale, world_projection);

    m4x4 translation = translation_matrix({-(float)room_size_x/2.0, -(float)room_size_y/2.0, 0});
    world_projection = matmul(translation, world_projection);

    return world_projection;
}


Ray screen_point_to_ray(v2 p, Rect viewport, m4x4 projection_inverse)
{
    /* Translate to "-1 -> 1 space"...*/
    p -= viewport.p;
    p  = compdiv(p, viewport.s / 2.0f);
    p -= V2_XY;
    
    /* Make 3D vector */
    v3 u = { p.x, p.y, 0 };

    /* Unproject */
    Ray ray;
    ray.p0  = vecmatmul(u, projection_inverse);
    ray.dir = normalize(vecmatmul(V3_Z, projection_inverse));

    return ray;
}

v2 world_to_screen_space(v3 p, Rect viewport, m4x4 projection)
{
    v2 result = vecmatmul(p, projection).xy;

    result += V2_XY;
    result  = compmul(result, viewport.s / 2.0f);
    result += viewport.p;

    return result;
}


// NOTE: tp is tile position.
Entity create_preview_item_entity(Item *item, v3 tp, double world_t, Quat q = Q_IDENTITY)
{
    v3 p = item_entity_p_from_tp(tp, item);
    
    Entity e = {0};
    *static_cast<S__Entity *>(&e) = create_item_entity(item, p, world_t, q);

    e.is_preview = true;
    return e;
}

void remove_preview_entities(Room *room)
{
    for(int i = 0; i < room->entities.n; i++)
    {
        auto *e = &room->entities[i];
        
        if(e->is_preview) {
            array_unordered_remove(room->entities, i);
            i--;
            continue;
        }
    }
}

Entity *add_entity(Entity e, Room *room)
{
    return array_add(room->entities, e);
}

Entity *find_entity(Entity_ID id, Room *room)
{
    for(int i = 0; i < room->entities.n; i++) {
        auto *e = &room->entities[i];
        if(e->id == id) return e;
    }

    return NULL;
}

Entity *find_or_add_entity(Entity_ID id, Room *room)
{
    Entity *found_entity = find_entity(id, room);
    if(found_entity) return found_entity;
    
    Entity new_entity = {0};
    new_entity.id = id;
    return add_entity(new_entity, room);
}

Entity *find_player_entity(User_ID user_id, Room *room)
{
    for(int i = 0; i < room->entities.n; i++) {
        auto *e = &room->entities[i];
        if(e->type != ENTITY_PLAYER) continue;
        if(e->player_e.user_id == user_id) return e;
    }

    return NULL;
}

// @Speed @Speed @Speed @Norelease
// IMPORTANT: There is one implementation for this for the client, and one for the room server.
//            Because C++ sucks. @Jai
Entity *item_entity_of_type_at(Item_Type_ID type, v3 p, double world_t, Room *room)
{
    for(int i = 0; i < room->entities.n; i++) {
        auto *e = room->entities.e + i;
        if(e->type != ENTITY_ITEM) continue;
        if(e->item_e.item.type != type) continue;

        if(entity_position(e, world_t, room) == p) {
            return e;
        }
    }
    return NULL;
}



void update_local_data_for_room(Room *room, double world_t, Client *client)
{
    User    *user    = current_user(client);
    User_ID  user_id = current_user_id(client);
    
    for(int i = 0; i < room->entities.n; i++)
    {
        auto *e = &room->entities[i];
        
        if(e->type == ENTITY_PLAYER)
        {
            auto *local = &e->player_local;
            local->is_me = (e->player_e.user_id == user_id);

            auto state = player_state_of(e, world_t, room);
            for(int j = 0; j < e->player_e.action_queue_length; j++)
            {
                auto *act = e->player_e.action_queue + j;

                // NOTE: Here we could also cache whether the action is predicted possible or not....
                
                apply_actions_to_player_state(&state, act, 1, world_t, room, user);
                local->state_after_action_in_queue[j] = state;
            }
            
        }
    }       
}

Player_State player_state_after_completed_action_queue(Entity *player, double world_t, Room *room)
{
    Assert(player->type == ENTITY_PLAYER);
    auto *player_e = &player->player_e;
    
    if(player_e->action_queue_length > 0) {
        return player->player_local.state_after_action_in_queue[player_e->action_queue_length-1];
    } else {
        return player_state_of(player, world_t, room);
    }
}



bool item_entity_can_be_at_tp(Entity *e, v3 tp, double world_t, Room *room)
{
    Assert(e->type == ENTITY_ITEM);
    return item_entity_can_be_at_tp(e, tp, world_t, room->entities.e, room->entities.n, room);
}

bool can_place_item_entity_at_tp(Item *item, v3 tp, double world_t, Room *room)
{
    return can_place_item_entity_at_tp(item, tp, world_t, room->entities.e, room->entities.n, room);
}


// IMPORTANT: The *_actions array should be in a valid state before calling this proc!
template<Allocator_ID A>
void get_available_actions_for_entity(Entity *e, Player_State *player_state, Array<Entity_Action, A> *_actions)
{
    _actions->n = 0;
    
    if(e->type != ENTITY_ITEM) return;

    Assert(e->type == ENTITY_ITEM);
    Item *item = &e->item_e.item;

    switch(item->type)
    {
        case ITEM_PLANT: {
            Entity_Action act1 = {0};
            act1.type = ENTITY_ACT_HARVEST;

            Entity_Action act2 = {0};
            act2.type = ENTITY_ACT_WATER;
            
            array_add(*_actions, act1);
            array_add(*_actions, act2);
        } break;

        case ITEM_MACHINE: {
            auto *machine_e = &e->item_e.machine;

            Entity_Action act1 = {0};
            act1.type = ENTITY_ACT_SET_POWER_MODE;
            act1.set_power_mode.set_to_on = (machine_e->stop_t >= machine_e->start_t);
                
            array_add(*_actions, act1);
        } break;

        case ITEM_CHESS_BOARD: {
            auto *board = &e->item_e.chess_board;

            Entity_Action act1 = {0};
            act1.type = ENTITY_ACT_CHESS;
            act1.chess.type = CHESS_ACT_JOIN;

            array_add(*_actions, act1);
        } break;

        case ITEM_CHAIR: {
            Entity_Action act1 = {0};
            act1.type = ENTITY_ACT_SIT_OR_UNSIT;
            act1.sit_or_unsit.unsit = (player_state->sitting_on == e->id);
            
            array_add(*_actions, act1);
        } break;
    }

    Entity_Action act_a = {0};
    act_a.type = ENTITY_ACT_PICK_UP;

    Entity_Action act_b = {0};
    act_b.type = ENTITY_ACT_PLACE_IN_INVENTORY;
    
    array_add(*_actions, act_a);
    array_add(*_actions, act_b);
}

// ///////////////////// //
