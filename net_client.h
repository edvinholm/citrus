
enum Room_Server_Connection_Status
{
    ROOM_SERVER_DISCONNECTED,
    ROOM_SERVER_CONNECTED,
};

enum C_RS_Action_Type
{
    C_RS_ACT_CLICK_TILE,
    C_RS_ACT_ENTITY_ACTION
};

struct C_RS_Action
{
    C_RS_Action_Type type;
    union {
        struct {
            s64 tile_ix;
            Item item_to_place;
        } click_tile;
        
        struct {
            Entity_ID     entity;
            Entity_Action action;
        } entity_action;
    };
};

// @Cleanup: Merge with User_Server_Connection in some way?
struct Room_Server_Connection
{
    Room_Server_Connection_Status status;
    Room_ID current_room;
    Network_Node node;

    bool last_connect_attempt_failed;
};
bool equal(Room_Server_Connection *a, Room_Server_Connection *b)
{
    if(a->status != b->status) return false;
    if(a->current_room != b->current_room) return false;
    if(!equal(&a->node.socket, &b->node.socket)) return false;
    
    if(a->last_connect_attempt_failed != b->last_connect_attempt_failed) return false;

    return true;
}


enum User_Server_Connection_Status
{
    USER_SERVER_DISCONNECTED,
    USER_SERVER_CONNECTED,
};

struct User_Server_Connection
{
    User_Server_Connection_Status status;
    User_ID current_user; // @Temporary: Should be an ID.
    Network_Node node;

    bool last_connect_attempt_failed;
};

// NOTE: This is only used in an assert as of 2021-01-23.
// @BadName: "totally" means that we for example require pointers to data to be the exact same pointer.
bool totally_equal(User_Server_Connection *a, User_Server_Connection *b)
{
    if(a->status != b->status) return false;
    
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
    Array<C_RS_Action, ALLOC_NETWORK> room_action_queue; // Writable by main loop
    // //// ////// //

    // USER SERVER //
    bool user_connect_requested; // Writable by main loop
    User_ID requested_user;      // Writable by main loop
    User_Server_Connection user;
    // //// ////// //
  
};
