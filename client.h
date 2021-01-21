
enum Client_Window_Type
{
    ROOM_LIST_WINDOW,
    USER_WINDOW,

    NUM_CLIENT_WINDOW_TYPES
};

typedef u64 Client_Window_ID;

struct Client_Window
{
    Client_Window_ID id;
    
    Client_Window_Type type;
    bool open;
};

struct Client_UI
{
    Array<Client_Window, ALLOC_APP> windows;
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

    Client_UI client_ui;
    
    // --
    User user;
    Game game;
    // --
    
    // @Norelease: Doing Developer stuff in release build...
    Developer developer;
};
