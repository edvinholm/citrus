
struct Client_UI
{
    bool room_window_open;
    bool user_window_open;
};

struct Client
{
    Mutex mutex;
    //--
    
    Layout_Manager layout;
    UI_Manager ui;
    Input_Manager input;

    Window main_window;
    Rect main_window_a;

    Font fonts[NUM_FONTS] = {0};

    // NETWORKING //
    Server_Connections server_connections;
    // --

    Client_UI cui;

    // --
    User user;
    Game game;
    // --
    
    // @Norelease: Doing Developer stuff in release build...
    Developer developer;
};
