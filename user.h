
struct S__User
{
    String username;
    v4 color;
};


namespace Client_User
{
    struct User
    {
        S__User shared;
    };
};


namespace Server_User
{
    struct User
    {
        S__User shared;
    };
};
