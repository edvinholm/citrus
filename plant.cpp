
bool is_plant(Entity *e)
{
    Assert(e->type == ENTITY_ITEM);
    return (e->item_e.item.type == ITEM_APPLE_TREE ||
            e->item_e.item.type == ITEM_WHEAT);
}

// NOTE: ITEM_NONE_OR_NUM means that the crop is the plant itself,
//       and that it should be copied as a new entity. The old entity's
//       grow_progress should then be set to zero.
Item_Type_ID crop_type_for_plant(Entity *e)
{
    Assert(is_plant(e));

    Item_Type_ID result = ITEM_NONE_OR_NUM;
    switch(e->item_e.item.type) {
        case ITEM_APPLE_TREE: result = ITEM_FRUIT;       break;
        case ITEM_WHEAT:      result = ITEM_NONE_OR_NUM; break;
        default: Assert(false); break;
    }

    return result;
}


Item_Type_ID plant_type_for_seed(Seed_Type seed)
{
    switch(seed) { // @Jai: #complete
        case SEED_APPLE: return ITEM_APPLE_TREE; break;
        case SEED_WHEAT: return ITEM_WHEAT;      break;
    }
    
    Assert(false);
    return ITEM_NONE_OR_NUM;
}
