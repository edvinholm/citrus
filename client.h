
enum Bottom_Panel_Tab
{
    BP_TAB_CHAT,

    BP_TAB_NONE_OR_NUM
};

String bottom_panel_tab_labels[] = {
    STRING("CHAT")
};
static_assert(ARRLEN(bottom_panel_tab_labels) == BP_TAB_NONE_OR_NUM);

struct Market_UI
{
    Item_Type_ID selected_item_type;
};

void reset(Market_UI *mui)
{
    mui->selected_item_type = ITEM_NONE_OR_NUM;
}

struct Client_UI
{
    bool room_window_open;
    bool user_window_open;
    bool market_window_open;

    Market_UI market;
    
#if DEVELOPER
    bool dev_window_open;
#endif

    Bottom_Panel_Tab open_bottom_panel_tab;
};

void init_client_ui(Client_UI *cui)
{
    cui->open_bottom_panel_tab = BP_TAB_NONE_OR_NUM;
}

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

User_ID current_user_id(Client *client);
User *current_user(Client *client);
