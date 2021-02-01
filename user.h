
typedef s64 Money;

struct S__User
{
    User_ID id;
    String username;
    v4 color;
    Money money; // @Norelease: This should only be shared with the client for this player, not other ones.

    Item inventory[8*14];
};
void clear(S__User *user, Allocator_ID allocator)
{
    clear(&user->username, allocator);
}


namespace Client_User
{
    struct User: public S__User
    {
        int selected_inventory_item_plus_one;
        Array<u8, ALLOC_APP> chat_draft;

        bool initialized;
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
    struct User: public S__User
    {
        bool did_change;
    };
};
