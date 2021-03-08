
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
    bool user_window_open;
    bool market_window_open;

    Market_UI market;
    Item_UI   item;
    
#if DEVELOPER
    Developer_UI dev;
#endif

    Bottom_Panel_Tab open_bottom_panel_tab;
};

void init_client_ui(Client_UI *cui)
{
    cui->open_bottom_panel_tab = BP_TAB_NONE_OR_NUM;
    
    cui->user_window_open = true;

    // @Norelease: @Temporary, I think. 
    cui->market.order_draft.price = 10;


#if DEVELOPER
    
#endif
}

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
    // --
    
    // @Norelease: Doing Developer stuff in release build...
    Developer developer;
};

User_ID current_user_id(Client *client);
User *current_user(Client *client);
Entity *find_current_player_entity(Client *client);
