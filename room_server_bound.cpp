
#ifndef ROOM_SERVER_BOUND_INCLUDED
#define ROOM_SERVER_BOUND_INCLUDED

enum RSB_Packet_Type
{
    RSB_HELLO   = 1,
    RSB_GOODBYE = 2,

    RSB_CLICK_TILE = 3,
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
            Item item_to_place;
        } click_tile;
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
            Read_To_Ptr(u64, &p->tile_ix, node);
            Read_To_Ptr(Item, &p->item_to_place, node);
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


bool enqueue_RSB_CLICK_TILE_packet(Network_Node *node, u64 tile_ix, Item item_to_place)
{
    begin_outbound_packet(node);
    {
        Write(RSB_Packet_Type, RSB_CLICK_TILE, node);
        //--

        Write(u64, tile_ix, node);
        Write(Item, item_to_place, node);
    }
    end_outbound_packet(node);
    return true;
}



#endif // ROOM_SERVER_BOUND_INCLUDED
