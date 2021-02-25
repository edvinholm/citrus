
#ifndef ROOM_SERVER_BOUND_INCLUDED
#define ROOM_SERVER_BOUND_INCLUDED

enum RSB_Packet_Type
{
    RSB_HELLO   = 1,
    RSB_GOODBYE = 2,

    RSB_CLICK_TILE = 3,
    RSB_PLAYER_ACTION = 4,
    RSB_CHAT = 5
};

// Room Server Bound Packet Header
struct RSB_Packet_Header
{
    RSB_Packet_Type type;
    union {
        struct {
            Room_ID room;
            User_ID as_user;
        } hello;
        
        struct {
        } goodbye;
        
        struct {
            u64 tile_ix;
            Item_ID item_to_place;

            //NOTE: This is only considered if item_to_place == NO_ITEM.
            //      If default_action_is_put_down is true, a "Put Down"
            //      action will be queued (if possible). Otherwise, a "Walk" action.
            bool default_action_is_put_down;
            
        } click_tile;

        Player_Action player_action;

        struct {
            String message_text;
        } chat;
    };
};



#define Write_RSB_Header(Packet_Type, Node_Ptr)    \
    Fail_If_True(!write_RSB_Packet_Header({Packet_Type}, Node_Ptr))


// @Norelease TODO: Check that it is a valid type.
bool read_RSB_Packet_Type(RSB_Packet_Type *_type, Network_Node *node)
{
    u64 i;
    if(!read_u64(&i, node)) return false;
    *_type = (RSB_Packet_Type)i;
    return true;
}


bool write_RSB_Packet_Type(RSB_Packet_Type type, Network_Node *node)
{
    return write_u64(type, node);
}



bool read_RSB_Packet_Header(RSB_Packet_Header *_header, Network_Node *node)
{
    Zero(*_header);
    Read_To_Ptr(RSB_Packet_Type, &_header->type, node);

    switch(_header->type)
    {
        case RSB_HELLO: {
            auto *p = &_header->hello;
            Read_To_Ptr(Room_ID, &p->room, node);
            Read_To_Ptr(User_ID, &p->as_user, node);
        } break;

        case RSB_GOODBYE: {} break;

        case RSB_CLICK_TILE: {
            auto *p = &_header->click_tile;
            Read_To_Ptr(u64,     &p->tile_ix, node);
            Read_To_Ptr(Item_ID, &p->item_to_place, node);
            Read_To_Ptr(bool,    &p->default_action_is_put_down, node);
        } break;

        case RSB_PLAYER_ACTION: {
            auto *p = &_header->player_action;
            Read_To_Ptr(Player_Action, p, node);
        } break;

        case RSB_CHAT: {
            auto *p = &_header->chat;
            Read_To_Ptr(String, &p->message_text, node);
        } break;
    }
    
    return true;
}


bool enqueue_RSB_HELLO_packet(Network_Node *node, Room_ID room, User_ID as_user)
{
    begin_outbound_packet(node);
    {
        Write(RSB_Packet_Type, RSB_HELLO, node);
        //--

        Write(Room_ID, room, node);
        Write(User_ID, as_user, node);
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_RSB_GOODBYE_packet(Network_Node *node)
{
    begin_outbound_packet(node);
    {
        Write(RSB_Packet_Type, RSB_GOODBYE, node);
        //--
    }
    end_outbound_packet(node);
    return true;
}

bool send_RSB_GOODBYE_packet_now(Network_Node *node)
{
    Send_Now_NoArgs(RSB_GOODBYE, node);
    return true;
}


bool enqueue_RSB_CLICK_TILE_packet(Network_Node *node, u64 tile_ix, Item_ID item_to_place, bool default_action_is_put_down)
{
    begin_outbound_packet(node);
    {
        Write(RSB_Packet_Type, RSB_CLICK_TILE, node);
        //--

        Write(u64,  tile_ix, node);
        Write(Item_ID, item_to_place, node);
        Write(bool, default_action_is_put_down, node);
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_RSB_PLAYER_ACTION_packet(Network_Node *node, Player_Action action)
{
    begin_outbound_packet(node);
    {
        Write(RSB_Packet_Type, RSB_PLAYER_ACTION, node);
        //--

        Write(Player_Action, action, node);
    }
    end_outbound_packet(node);
    return true;
}


bool enqueue_RSB_CHAT_packet(Network_Node *node, String message_text)
{    
    begin_outbound_packet(node);
    {
        Write(RSB_Packet_Type, RSB_CHAT, node);
        //--

        Write(String, message_text, node);
    }
    end_outbound_packet(node);
    return true;
}

#endif // ROOM_SERVER_BOUND_INCLUDED
