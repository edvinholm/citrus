
enum Room_Connect_Status: u64 // @Cleanup: This does not need to be 64 bits (Change this here and in network code)
{
    ROOM_CONNECT__REQUEST_RECEIVED = 1,
    ROOM_CONNECT__CONNECTED        = 2,
    
    ROOM_CONNECT__INVALID_ROOM_ID  = 3
};

enum RCB_Packet_Type
{
    RCB_HELLO     = 1,
    RCB_GOODBYE   = 2,
    
    RCB_ROOM_INIT   = 3,
    RCB_ROOM_UPDATE = 4,

    RCB_PACKET_RESULT = 255
};

struct RCB_Packet_Result_Payload
{
    union {
        Player_Action_ID enqueued_action_id;
    };
};

struct RCB_Packet_Header
{
    RCB_Packet_Type type;

    union {
        struct {
            Room_Connect_Status connect_status;
        } hello;
        
        struct {} goodbye;
        
        struct {
            World_Time time;
            u32 num_entities;
            Tile *tiles;
            Walk_Map walk_map;
        } room_init;
        
        struct {
            u32 tile0;
            u32 tile1;
            u32 num_entities;
            u16 num_chat_messages;
            Tile *tiles;
            Walk_Map_Node *walk_map_nodes;
        } room_update;


        struct {
            RSB_Packet_Type rsb_packet_type;
            u32             rsb_packet_id;
            bool success;

            RCB_Packet_Result_Payload payload;
            
        } packet_result;
    };
};
