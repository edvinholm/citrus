
enum C_RS_Action_Type
{
    C_RS_ACT_CLICK_TILE, // @Cleanup: Should be in player action. (?)
    C_RS_ACT_PLAYER_ACTION,
    C_RS_ACT_PLAYER_ACTION_DEQUEUE,
    C_RS_ACT_CHAT
};

struct C_RS_Action
{
    C_RS_Action_Type type;
    union {
        struct {
            s64 tile_ix;
        } click_tile;

        Player_Action player_action;

        struct {
            Player_Action_ID action_id;
        } player_action_dequeue;

        struct {
            
        } chat;
    };
};


enum C_MS_Action_Type
{
    C_MS_PLACE_ORDER,
    C_MS_SET_VIEW
};

struct C_MS_Action
{
    C_MS_Action_Type type;
    union {
        struct {
            Money price;
            bool is_buy_order; // Otherwise it is a sell order.
            union {
                struct {
                    Item_Type_ID item_type;
                } buy;

                struct {
                    Item_ID item_id;
                } sell;
            };
        } place_order;
        
        struct {
            Market_View_Target target;
        } set_view;
    };
};



// @Cleanup: Merge with User_Server_Connection in some way?
struct Room_Server_Connection
{
    bool connected;
    Room_ID current_room;
    Network_Node node;

    bool last_connect_attempt_failed;
};
bool equal(Room_Server_Connection *a, Room_Server_Connection *b)
{
    if(a->connected != b->connected) return false;
    if(a->current_room != b->current_room) return false;
    if(!equal(&a->node.socket, &b->node.socket)) return false;
    
    if(a->last_connect_attempt_failed != b->last_connect_attempt_failed) return false;

    return true;
}

struct User_Server_Connection
{
    bool connected;
    User_ID current_user;
    Network_Node node;

    bool last_connect_attempt_failed;
};

// NOTE: This is only used in an assert as of 2021-02-04.
// @BadName: "totally" means that we for example require pointers to data to be the exact same pointer.
bool totally_equal(User_Server_Connection *a, User_Server_Connection *b)
{
    if(a->connected != b->connected) return false;
    
    if(a->current_user != b->current_user) return false;

    if(!equal(&a->node.socket, &b->node.socket)) return false;
    
    if(a->last_connect_attempt_failed != b->last_connect_attempt_failed) return false;

    return true;
};



struct Market_Server_Connection
{
    bool connected;
    User_ID current_user;
    Network_Node node;

    bool last_connect_attempt_failed;
};

// NOTE: This is only used in an assert as of 2021-02-04.
// @BadName: "totally" means that we for example require pointers to data to be the exact same pointer.
bool totally_equal(Market_Server_Connection *a, Market_Server_Connection *b)
{
    if(a->connected != b->connected) return false;
    
    if(a->current_user != b->current_user) return false;

    if(!equal(&a->node.socket, &b->node.socket)) return false;
    
    if(a->last_connect_attempt_failed != b->last_connect_attempt_failed) return false;

    return true;
};



struct Server_Connections
{
    // ROOM SERVER //
    bool room_connect_requested; // Writable by main loop
    Room_ID requested_room;      // Writable by main loop
    Room_Server_Connection room;
    Array<C_RS_Action, ALLOC_MALLOC> room_action_queue; // Writable by main loop
    // //// ////// //

    // USER SERVER //
    bool user_connect_requested; // Writable by main loop
    User_ID requested_user;      // Writable by main loop
    User_Server_Connection user;
    // //// ////// //

    // MARKET SERVER //
    bool market_connect_requested; // Writable by main loop
    Market_Server_Connection market;
    Array<C_MS_Action, ALLOC_MALLOC> market_action_queue; // Writable by main loop
    // ////// ////// //
};
