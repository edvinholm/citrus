
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


Mesh_ID mesh_for_entity(Entity *e)
{
    if(e->type != ENTITY_ITEM) return MESH_NONE_OR_NUM;

    switch(e->item_e.item.type) {
        case ITEM_BED:          return MESH_BED;
        case ITEM_CHAIR:        return MESH_CHAIR;
        case ITEM_BLENDER:      return MESH_BLENDER;
        case ITEM_TABLE:        return MESH_TABLE;
        case ITEM_BARREL:       return MESH_BARREL;
        case ITEM_FILTER_PRESS: return MESH_FILTER_PRESS;
        case ITEM_STOVE:        return MESH_STOVE;
        case ITEM_GRINDER:      return MESH_GRINDER;
        case ITEM_TOILET:       return MESH_TOILET;

        default: return MESH_NONE_OR_NUM;
    }
}
        

void raycast_against_entities_and_surfaces(Ray ray, Room *room, double world_t, Asset_Catalog *assets,
                                           Entity **_hit_entity, v3 *_entity_hit_p = NULL,
                                           Optional<Surface> *_hit_surface = NULL, v3 *_surface_hit_p = NULL)
{
    Entity *closest_entity   = NULL;
    float   closest_entity_ray_t = FLT_MAX;
    
    Optional<Surface> closest_surface = {0};
    float   closest_surface_ray_t = FLT_MAX;

    for(int i = 0; i < room->entities.n; i++)
    {
        auto *e = &room->entities[i];

        v3 p;
        Quat q;
        get_entity_transform(e, world_t, &p, &q, room);
    
        auto hitbox = entity_hitbox(e, p, q);
        
        v3 bbox_intersection;
        float bbox_ray_t;
        if(ray_intersects_aabb(ray, hitbox.base, &bbox_intersection, &bbox_ray_t))
        {
            // ENTITY //
            if(bbox_ray_t < closest_entity_ray_t) {
                bool did_hit = false;
                
                v3 intersection = bbox_intersection;
                float ray_t = bbox_ray_t;
       
                Mesh_ID mesh = mesh_for_entity(e);
                if(mesh != MESH_NONE_OR_NUM) {
                    m4x4 m = rotation_matrix(q) * translation_matrix(p);
                    m4x4 inverse = inverse_of(m);
                        
                    Ray mesh_space_ray;
                    mesh_space_ray.p0  = vecmatmul(ray.p0,  inverse);
                    mesh_space_ray.dir = vecmatmul(ray.dir, inverse, 0);
                        
                    v3 mesh_space_intersection;
                    if(ray_intersects_mesh(mesh_space_ray, &assets->meshes[mesh], &mesh_space_intersection, &ray_t)) {
                        did_hit = true;
                        intersection = vecmatmul(mesh_space_intersection, m);
                    }                        
                }
                else did_hit = true;

                if(did_hit && ray_t < closest_entity_ray_t) {
                    closest_entity       = e;
                    closest_entity_ray_t = ray_t;
                        
                    if(_entity_hit_p) *_entity_hit_p = intersection;
                }
            }

            // SURFACES //
            if(bbox_ray_t < closest_surface_ray_t) {
                auto surfaces = item_entity_surfaces(e, world_t, room);
                for(int i = 0; i < surfaces.n; i++) {
                    auto &surf = surfaces[i];

                    v3 quad_a = surf.p;
                    v3 quad_d = surf.p + V3(surf.s.x, surf.s.y, 0);
                    
                    v3 quad_b = quad_a;
                    quad_b.x  = quad_d.x;
                    
                    v3 quad_c = quad_a;
                    quad_c.y  = quad_d.y;

                    v3 intersection;
                    float ray_t;
                    if(!ray_intersects_quad(ray, quad_a, quad_b, quad_c, quad_d, &intersection, &ray_t)) continue;

                    if(!closest_surface.present ||
                       ray_t < closest_surface_ray_t)
                    {
                        closest_surface = surf;
                        closest_surface_ray_t = ray_t;

                        if(_surface_hit_p) *_surface_hit_p = intersection;
                    }
                }
            }                
        }
    }

    *_hit_entity  = closest_entity;
    *_hit_surface = closest_surface;
}

bool raycast_against_floor(Ray ray, v3 *_hit)
{
    if(is_zero(ray.dir.z)) return false;
    
    *_hit = ray.p0 + ray.dir * (-ray.p0.z / ray.dir.z);
    return true;
}

m4x4 world_projection_matrix(v2 viewport_s, float room_sx, float room_sy, float room_sz, float z_offset/* = 0*/, bool apply_tweaks/* = true*/)
{
    float x_mul, y_mul;
    x_mul = 1;
    y_mul = 1.0f/(viewport_s.h / max(0.0001f, viewport_s.w));

    // TODO @Speed: @Cleanup: Combine matrices

    float z_mul = -0.01;

    float dx = 0;
    float dy = 0;
    if(apply_tweaks) {
        v2 cam_trans_offs = tweak_v2(TWEAK_CAMERA_TRANSLATION_OFFSET);
        dx = -cam_trans_offs.x * x_mul;
        dy = -cam_trans_offs.y * y_mul;
    }

    m4x4 m = make_m4x4(
        x_mul, 0, 0, dx,
        0, y_mul, 0, dy,
        0, 0, z_mul, z_offset,
        0, 0, 0, 1);

    
    v3 cam_rot_offs;
    if(apply_tweaks) {
        cam_rot_offs = tweak_v3(TWEAK_CAMERA_ROTATION_OFFSET);
    } else {
        cam_rot_offs = V3_ZERO;
    }

    m4x4 rotation = rotation_matrix(axis_rotation(V3_X, TAU * (-0.125 + cam_rot_offs.x / 360.0 )));
    m = matmul(rotation, m);

    rotation = rotation_matrix(axis_rotation(V3_Y, TAU * (0 + cam_rot_offs.y / 360.0)));
    m = matmul(rotation, m);
    
    rotation = rotation_matrix(axis_rotation(V3_Z, TAU * (0.125 + cam_rot_offs.z / 360.0)));
    m = matmul(rotation, m);

    float height;
    float scale_f = 1.0f;
    {
        v3 p1 = V3_ZERO;
        v3 p2 = { 0,       room_sy, 0 };
        v3 p3 = { room_sx,       0, 0 };
        v3 p4 = { room_sx, room_sy, 0 };
        
        v3 p5 = p4;
        p5.z += room_sz;

        p1 = vecmatmul(p1, m);
        p2 = vecmatmul(p2, m);
        p3 = vecmatmul(p3, m);
        p4 = vecmatmul(p4, m);
        p5 = vecmatmul(p5, m);

        float min_x = min(min(min(min(p1.x, p2.x), p3.x), p4.x), p5.x);
        float max_x = max(max(max(max(p1.x, p2.x), p3.x), p4.x), p5.x);
        
        float min_y = min(min(min(min(p1.y, p2.y), p3.y), p4.y), p5.y);
        float max_y = max(max(max(max(p1.y, p2.y), p3.y), p4.y), p5.y);

        height = (max_y - min_y);
        scale_f = min(2.0 / (max_x-min_x), 2.0 / height);
    }


    if(apply_tweaks) {
        float cam_scale_mul = tweak_float(TWEAK_CAMERA_SCALE_MULTIPLIER);
        if(cam_scale_mul > 0) scale_f *= cam_scale_mul;
    }
    
    m4x4 scale = scale_matrix(V3_ONE * scale_f);
    m = matmul(scale, m);

    m4x4 translation = translation_matrix({-room_sx/2.0f, -room_sy/2.0f, -room_sz/2.0f});
    m = matmul(translation, m);
    
    return m;
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
    ray.dir = normalize(vecmatmul(V3_Z, projection_inverse, 0));

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

Entity create_preview_item_entity(Item *item, v3 p, Quat q, double world_t)
{
    Entity e = {0};
    *static_cast<S__Entity *>(&e) = create_item_entity(item, p, q, world_t);

    e.is_preview = true;
    return e;
}

// NOTE: tp is tile position.
Entity create_preview_item_entity_at_tp(Item *item, v3 tp, Quat q, double world_t)
{
    v3 p = item_entity_p_from_tp(tp, item, q);
    return create_preview_item_entity(item, p, q, world_t);
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

Entity *find_or_add_entity(Entity_ID id, Room *room)
{
    Entity *found_entity = find_entity(id, room);
    if(found_entity) return found_entity;
    
    Entity new_entity = {0};
    new_entity.id = id;
    return add_entity(new_entity, room);
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
            local->state_before_action_in_queue[0] = state;
            for(int j = 0; j < e->player_e.action_queue_length; j++)
            {
                auto *act = e->player_e.action_queue + j;

                // NOTE: Here we could also cache whether the action is predicted possible or not....
                
                apply_actions_to_player_state(&state, act, 1, world_t, room, user);
                local->state_before_action_in_queue[j+1] = state;
            }
            
        }
    }       
}

Player_State player_state_after_completed_action_queue(Entity *player, double world_t, Room *room)
{
    Assert(player->type == ENTITY_PLAYER);
    auto *player_e = &player->player_e;
    
    return player->player_local.state_before_action_in_queue[player_e->action_queue_length];
}


// IMPORTANT: The *_actions array should be in a valid state before calling this proc!
template<Allocator_ID A>
void get_available_actions_for_entity(Entity *e, Player_State *player_state, Array<Entity_Action, A> *_actions, bool *_first_action_is_default = NULL)
{
    _actions->n = 0;
    bool first_is_default = false;
    
    if(e->type != ENTITY_ITEM) return;

    Assert(e->type == ENTITY_ITEM);
    Item *item = &e->item_e.item;

    switch(item->type)
    {
        case ITEM_APPLE_TREE:
        case ITEM_WHEAT: {
            Entity_Action act1 = {0};
            act1.type = ENTITY_ACT_HARVEST;

            Entity_Action act2 = {0};
            act2.type = ENTITY_ACT_WATER;
            
            array_add(*_actions, act1);
            array_add(*_actions, act2);

            first_is_default = true;
        } break;

        case ITEM_MACHINE: {
            auto *machine_e = &e->item_e.machine;

            Entity_Action act1 = {0};
            act1.type = ENTITY_ACT_SET_POWER_MODE;
            act1.set_power_mode.set_to_on = (machine_e->stop_t >= machine_e->start_t);
                
            array_add(*_actions, act1);
            
            first_is_default = true;
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
            
            first_is_default = (!act1.sit_or_unsit.unsit);
        } break;

        case ITEM_BED: {
            Entity_Action act1 = {0};
            act1.type = ENTITY_ACT_SLEEP;
            
            array_add(*_actions, act1);
            
            first_is_default = true;
        } break;
            
        case ITEM_TOILET: {
            Entity_Action act1 = {0};
            act1.type = ENTITY_ACT_USE_TOILET;
            
            array_add(*_actions, act1);
            
            first_is_default = true;
        } break;
    }

    Entity_Action act_a = {0};
    act_a.type = ENTITY_ACT_PICK_UP;

    Entity_Action act_b = {0};
    act_b.type = ENTITY_ACT_PLACE_IN_INVENTORY;
    
    array_add(*_actions, act_a);
    array_add(*_actions, act_b);

    if(_first_action_is_default) *_first_action_is_default = first_is_default;
}

// ///////////////////// //
