
typedef s64 Money;

enum Inventory_Slot_Flag_
{
    INV_SLOT_FILLED   = 0x01,
    INV_SLOT_RESERVED = 0x02 // If this is set, the slot cannot be modified by the player, and:
                             //   - if INV_SLOT_FILLED is not set, it is waiting for the item to fill it.
                             //   - if INV_SLOT_FILLED is     set, the item counts as being in the slot, but locked.
                             // Inventory_Slot.item is always valid if INV_SLOT_RESERVED is set:
                             //   - if INV_SLOT_FILLED is not set, .item is what the item was when we last saw it, or what someone told us it is.
                             //   - if INV_SLOT_FILLED is     set, .item is the actual item.
};

typedef u8 Inventory_Slot_Flags;

struct Inventory_Slot
{
    Inventory_Slot_Flags flags;
    Item item;
};

struct S__User
{
    User_ID id;
    String username;
    v4 color;
    
    Money money; // @Norelease: This should only be shared with the client for this player, not other ones.
    Money reserved_money;

    Inventory_Slot inventory[8*14];
};
void clear(S__User *user, Allocator_ID allocator)
{
    clear(&user->username, allocator);
}


namespace Client_User
{
    struct User: public S__User
    {
        bool initialized;
        
        int selected_inventory_item_plus_one;
        Array<u8, ALLOC_APP> chat_draft;
    };

    void clear_and_reset(User *user, Allocator_ID allocator)
    {
        clear(static_cast<S__User *>(user), allocator);

        clear(&user->chat_draft);

        Zero(*user);
    }
};


namespace Server_User
{
    struct Money_Reservation
    {
        u32 server_id;
        Money amount;
    };
    
    struct User: public S__User
    {
        u32 inventory_reservations[ARRLEN(S__User::inventory)]; // This contains the server IDs that has reserved the mapped inventory slots.

        Money total_money_reserved;
        Array<Money_Reservation, ALLOC_MALLOC> money_reservations;
        
        bool did_change;
    };

    void clear(User *user, Allocator_ID allocator)
    {
        clear(&user->money_reservations);
        
        clear(static_cast<S__User *>(user), allocator);
    }
};
