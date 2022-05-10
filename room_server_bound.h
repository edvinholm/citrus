
enum RSB_Packet_Type
{
    RSB_HELLO   = 1,
    RSB_GOODBYE = 2,

    RSB_CLICK_TILE = 3,
    RSB_PLAYER_ACTION = 4,
    RSB_CHAT = 5,

    RSB_PLAYER_ACTION_DEQUEUE = 6,
    RSB_PLAYER_ACTION_QUEUE_PAUSE = 7,


    RSB_DEVELOPER = 8
};

enum RSB_Developer_Packet_Type
{
    RSB_DEV_ADD_ENTITY
};

struct RSB_Developer_Packet
{
    RSB_Developer_Packet_Type type;
    union {
        struct {
            S__Entity entity; // (ID will be overwritten by server)
        } add_entity;
    };    
};

// Room Server Bound Packet Header
struct RSB_Packet_Header
{
    RSB_Packet_Type type;
    
    // The client decides what this is. Does not have to be unique among
    // all RSBs from all clients. Doesn't even have to be unique among
    // its client's packets. Can be set to zero for packets where no
    // RSB_PACKET_RESULT is expected.
    u32 id; 
    
    union {
        struct {
            Room_ID room;
            User_ID as_user;
        } hello;
        
        struct {
        } goodbye;
        
        struct {
            u64 tile_ix;            
        } click_tile;

        Player_Action player_action;

        struct {
            Player_Action_ID action_id;
        } player_action_dequeue;
        
        struct {
            bool remove;
            Player_Action_ID action_after; // If this is 0, it is at the end of the queue.
        } player_action_queue_pause;

        struct {
            String message_text;
        } chat;

        RSB_Developer_Packet developer;
    };
};
