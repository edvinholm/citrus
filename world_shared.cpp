


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

            plant->grow_progress = state->grow_progress_on_plant + (world_t - state->t_on_plant) * (1.0f / 60.0f);
        } break;
    }
}

v3 item_entity_p_from_tp(v3 tp, Item *item)
{
    v3 p = tp;
    
    v3s volume = item_types[item->type].volume;
    if(volume.x % 2 != 0) p.x += 0.5f;
    if(volume.y % 2 != 0) p.y += 0.5f;

    return p;
}

S__Entity create_item_entity(Item *item, v3 p, double world_t)
{
    S__Entity e = {0};
    e.type = ENTITY_ITEM;
    e.item_e.item = *item;
    e.item_e.p    = p;

    switch(e.item_e.item.type) {
        case ITEM_PLANT: {
            auto *plant_e = &e.item_e.plant;
            plant_e->t_on_plant = world_t;
            plant_e->grow_progress_on_plant = item->plant.grow_progress;
        } break;
    }

    return e;
}

v3 entity_position(S__Entity *e, double world_t)
{
    switch(e->type) {
        case ENTITY_ITEM: {        
            return e->item_e.p;
        } break;
            
        case ENTITY_PLAYER:
        {
            float walk_speed = 4.2f;
            
            auto p0 = e->player_e.walk_p0;
            auto p1 = e->player_e.walk_p1;
            auto t0 = e->player_e.walk_t0;

            auto x = walk_speed * ((world_t - t0) / magnitude(p1 - p0));
            return lerp(p0, p1, clamp(x));
            
        } break;

        default: Assert(false); return V3_ZERO;
    }

    Assert(false);
    return V3_ZERO;
}

AABB entity_aabb(S__Entity *e, double world_t)
{
    AABB bbox = {0};
    auto p = entity_position(e, world_t);

    switch(e->type) {
        case ENTITY_ITEM: {
            auto *item_e = &e->item_e;
            Assert(item_e->item.type != ITEM_NONE_OR_NUM);
    
            auto *type = &item_types[item_e->item.type];
    
            bbox.s = { (float)type->volume.x, (float)type->volume.y, (float)type->volume.z };
        } break;

        case ENTITY_PLAYER: {
            bbox.s = { 2, 2, 4 };
        } break;

        default: Assert(false); break;
    }
    
    bbox.p.xy = p.xy - bbox.s.xy / 2.0f;
    bbox.p.z  = p.z;

    return bbox;
}


template<typename ENTITY>
bool can_place_item_entity(Item *item, v3 p, double world_t, S__Room *room, ENTITY *entities, s64 num_entities)
{
    S__Entity my_entity = create_item_entity(item, p, world_t);
    AABB my_bbox = entity_aabb(&my_entity, world_t);
    
    for(int i = 0; i < num_entities; i++)
    {
        auto *e = entities + i;
        AABB other_bbox = entity_aabb(&e->shared, world_t);
        
        if(aabb_intersects_aabb(my_bbox, other_bbox))
            return false;
    }
    
    return true;
}

template<typename ENTITY>
bool can_place_item_entity_at_tp(Item *item, v3 tp, double world_t, S__Room *room, ENTITY *entities, s64 num_entities)
{
    return can_place_item_entity(item, item_entity_p_from_tp(tp, item), world_t, room, entities, num_entities);
}
