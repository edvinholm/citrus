

enum C_RS_Action_Type
{
    C_RS_ACT_CLICK_TILE, // @Cleanup: Should be in player action. (?)
    C_RS_ACT_PLAYER_ACTION,
    C_RS_ACT_PLAYER_ACTION_DEQUEUE,
    C_RS_ACT_PLAYER_ACTION_QUEUE_PAUSE,
    C_RS_ACT_CHAT,

#if DEVELOPER
    C_RS_ACT_DEVELOPER
#endif
};


struct C_RS_Action
{
    C_RS_Action_Type type;
    union {
        struct {
            s64 tile_ix;
        } click_tile;

        Pending_Player_Action player_action;

        struct {
            Player_Action_ID action_id;
        } player_action_dequeue;
        
        struct {
            bool remove;
            int ix_of_action_after;
        } player_action_queue_pause;

        struct {
            
        } chat;

        RSB_Developer_Packet developer_packet;
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
    Network_Node node;
    
    Room_ID current_room;
    u32 prev_rsb_packet_id; // NOTE: Not all RSB types use IDs => This will not always increase for every packet.

};
bool equal(Room_Server_Connection *a, Room_Server_Connection *b)
{
    if(!equal(&a->node, &b->node)) return false;
    if(a->current_room != b->current_room) return false;
    return true;
}

struct User_Server_Connection
{
    Network_Node node;
 
    User_ID current_user;
};

// NOTE: This is only used in an assert as of 2021-02-04.
// @BadName: "totally" means that we for example require pointers to data to be the exact same pointer.
bool totally_equal(User_Server_Connection *a, User_Server_Connection *b)
{
    if(!equal(&a->node, &b->node)) return false;    
    if(a->current_user != b->current_user) return false;
    return true;
};



struct Market_Server_Connection
{
    Network_Node node;
    
    User_ID current_user;
};

// NOTE: This is only used in an assert as of 2021-02-04.
// @BadName: "totally" means that we for example require pointers to data to be the exact same pointer.
bool totally_equal(Market_Server_Connection *a, Market_Server_Connection *b)
{
    if(!equal(&a->node, &b->node)) return false;
    if(a->current_user != b->current_user) return false;
    return true;
};



struct Server_Connections
{
#if 0
    // MASTER SERVER //
    bool master_connect_requested; // Writable by main loop
    Master_Server_Connection master;
    // ////// ////// //
#endif
    
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
