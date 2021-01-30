

Item *find_first_empty_inventory_slot(S__User *user)
{
    for(int i = 0; i < ARRLEN(user->inventory); i++) {
        auto *slot = &user->inventory[i];
        if(slot->id == NO_ITEM) return slot;
    }
    return NULL;
}

bool inventory_has_available_space_for_item(Item *item , S__User *user)
{
    return find_first_empty_inventory_slot(user) != NULL;
}
