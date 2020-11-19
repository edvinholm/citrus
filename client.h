    
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
    
    // --
    Game game;
    // --
    
    // @Norelease: Doing Developer stuff in release build...
    Developer developer;
};
