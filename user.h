
typedef s64 Money;

struct S__User
{
    User_ID id;
    String username;
    v4 color;
    Money money; // @Norelease: This should only be shared with the client for this player, not other ones.

    Item inventory[8*14];
};


namespace Client_User
{
    struct User: public S__User
    {
        int selected_inventory_item_plus_one;
    };
};


namespace Server_User
{
    struct User: public S__User
    {
        bool did_change;
    };
};
