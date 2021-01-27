
// INVENTORY //

Item *get_selected_inventory_item(User *user)
{
    if(user->selected_inventory_item_plus_one == 0) return NULL;
    return &user->shared.inventory[user->selected_inventory_item_plus_one-1];
}

void inventory_deselect(User *user)
{
    user->selected_inventory_item_plus_one = 0;
}

void empty_inventory_slot_locally(int slot_ix, User *user)
{
    Assert(slot_ix > 0);
    Assert(slot_ix < ARRLEN(user->shared.inventory));

    auto *slot = &user->shared.inventory[slot_ix];

    // @Boilerplate: User Server: commit_transaction().
    Zero(*slot);
    slot->id = NO_ITEM;
    slot->type = ITEM_NONE_OR_NUM;
}

void inventory_remove_item_locally(Item_ID id, User *user)
{
    for(int i = 0; i < ARRLEN(user->shared.inventory); i++)
    {
        auto *item = &user->shared.inventory[i];
        if(item->id == id) {
            empty_inventory_slot_locally(i, user);
            return;
        }
    }

    Assert(false);
}

// //////// //



// WORLD //

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

    m4x4 rotation = rotation_matrix(axis_rotation(V3_X, PIx2 * 0.125));
    world_projection = matmul(rotation, world_projection);

    rotation = rotation_matrix(axis_rotation(V3_Z, PIx2 * -0.125));
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
    p.y *= -1;
    
    /* Make 3D vector */
    v3 u = { p.x, p.y, 0 };

    /* Unproject */
    Ray ray;
    ray.p0  = vecmatmul(u, projection_inverse);
    ray.dir = normalize(vecmatmul(V3_Z, projection_inverse));

    return ray;
}


v3 tp_from_index(s32 tile_index)
{
    int y = tile_index / room_size_x;
    int x = tile_index % room_size_x;
    return { (float)x, (float)y, 0 };
}


// NOTE: tp is tile position.
Entity create_preview_item_entity(Item *item, v3 tp)
{
    Entity e = {0};
    e.shared.type = ENTITY_ITEM;
    e.shared.p = tp;
    e.shared.item = *item;

    e.is_preview = true;
    
    v3s volume = item_types[item->type].volume;
    if(volume.x % 2 != 0) e.shared.p.x += 0.5f;
    if(volume.y % 2 != 0) e.shared.p.y += 0.5f;

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

Entity *find_or_add_entity(Entity_ID id, Room *room)
{
    for(int i = 0; i < room->entities.n; i++) {
        auto *e = &room->entities[i];
        if(e->shared.id == id) return e;
    }

    Entity new_entity = {0};
    new_entity.shared.id = id;
    return add_entity(new_entity, room);
}

// ///////////////////// //
