
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
    struct {
        Money price;
    } order_draft;
};

void reset(Market_UI *mui)
{
    
}

enum Item_Window_Tab
{
    ITEM_TAB_INFO = 0,
    ITEM_TAB_CHESS,

    ITEM_TAB_NONE_OR_NUM
};

String item_window_tab_labels[] = {
    STRING("i"),
    STRING("GAME")
};
static_assert(ARRLEN(item_window_tab_labels) == ITEM_TAB_NONE_OR_NUM);

struct Item_UI
{
    Item_Window_Tab tab;
};

struct Client_UI
{
    bool room_window_open;
    bool tools_window_open;
    bool user_window_open;
    bool market_window_open;
    bool needs_window_open;

    Market_UI market;
    Item_UI   item;

    // TOOLS //
    Tool_ID current_tool;
    Tool_ID last_tool;
    double tool_switch_t;
    Entity_Menu   entity_menu;
    Planting_Tool planting_tool;

#if DEVELOPER
    Room_Editor   room_editor;
#endif
    // ///// //
    
#if DEVELOPER
    Developer_UI dev;
#endif

    Bottom_Panel_Tab open_bottom_panel_tab;
    
    UI_Dock dock;
};

struct Market
{
    bool initialized;

    // This is a little @Hacky. @Cleanup.
    bool ui_needs_update; // NOTE: This is set to true to tell the UI code to for example update the price textfield after the watched article changed. It is then set to false by the UI code.

    bool waiting_for_view_update; // @Norelease: What happens if we set this to true and then something goes wrong and we never get a MARKET_UPDATE from the server?  -EH, 2021-02-10
    Market_View view;
};

void clear_and_reset(Market *market) {
    Zero(*market);

    auto *view = &market->view;
    Zero(*view);
    view->target.type = MARKET_VIEW_TARGET_ARTICLE;
    view->target.article.article = ITEM_NONE_OR_NUM;
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

    // ASSETS //
    Asset_Catalog assets;
    Font_Table fonts;
    // --

    // NETWORKING //
    Server_Connections connections;
    // --

    Client_UI cui;

    // --
    User   user;
    Room   room;
    Market market;
    User_Utilities utilities;
    // --
    
    // @Norelease: Doing Developer stuff in release build...
    Developer developer;
};

User_ID current_user_id(Client *client);
User *current_user(Client *client);
Entity *find_current_player_entity(Client *client);


bool predicted_possible(Player_Action *action, double world_t, Player_State player_state, Client *client, Action_Prediction_Info *_prediction_info = NULL);
bool request_if_predicted_possible(Player_Action *action, double world_t, Player_State player_state, Client *client);
Pending_Player_Action *request(Entity_Action *action, Entity_ID target, Client *client);
bool predicted_possible(Entity_Action *action, Entity_ID target, double world_t, Player_State player_state, Client *client);
bool request_if_predicted_possible(Entity_Action *action, Entity_ID target, double world_t, Player_State player_state, Client *client);

