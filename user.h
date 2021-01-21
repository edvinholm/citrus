
struct S__User
{
    String username;
    v4 color;

    Item_Type_ID inventory[8*14];
};


namespace Client_User
{
    struct User
    {
        S__User shared;

        int selected_inventory_item_plus_one;
    };
};


namespace Server_User
{
    struct User
    {
        S__User shared;
    };
};
