

Inventory_Slot *find_first_empty_inventory_slot(S__User *user)
{
    for(int i = 0; i < ARRLEN(user->inventory); i++) {
        auto *slot = &user->inventory[i];

        if(slot->flags & INV_SLOT_FILLED)   continue;
        if(slot->flags & INV_SLOT_RESERVED) continue;

        return slot;
    }
    return NULL;
}

bool inventory_has_available_space_for_item_type(Item_Type_ID item_type , S__User *user)
{
    return find_first_empty_inventory_slot(user) != NULL;
}
