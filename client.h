
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
    
    cui->user_window_open = true;

    // @Norelease: @Temporary, I think. 
    cui->market.order_draft.price = 10;
}

// @Cleanup: Make an S__Market_Article maybe. To have as the watched article here.
struct Market
{
    bool initialized;
    bool waiting_for_watched_article_to_be_set;

    Item_Type_ID watched_article;

    // NOTE: price history is only valid if watched_article != ITEM_NONE_OR_NUM
    u16 price_history_length_for_watched_article;
    Money price_history_for_watched_article[10]; // @Norelease: @Robustness: This length must be the same as it is on the server.
};

void clear_and_reset(Market *market) {
    Zero(*market);

    market->watched_article = ITEM_NONE_OR_NUM;
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
