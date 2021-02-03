
#ifndef MARKET_SERVER_BOUND_INCLUDED
#define MARKET_SERVER_BOUND_INCLUDED


enum MSB_Packet_Type
{
    MSB_HELLO = 1,
    MSB_GOODBYE = 2,

    MSB_PLACE_ORDER
};

// User Server Bound Packet Header
struct MSB_Packet_Header
{
    MSB_Packet_Type type;
    union {
        struct {
            User_ID user;
            MS_Client_Type client_type;
        } hello;
        
        struct {
        } goodbye;

        struct {
            Item_Type_ID item_type;
            Money        price;
            bool         is_buy_order; // If false, it is a sell order.
        } place_order;
    };
};


bool read_MSB_Packet_Type(MSB_Packet_Type *_type, Network_Node *node)
{
    u64 i;
    if(!read_u64(&i, node)) return false;
    *_type = (MSB_Packet_Type)i;
    return true;
}


bool write_MSB_Packet_Type(MSB_Packet_Type type, Network_Node *node)
{
    return write_u64(type, node);
}



bool read_MS_Client_Type(MS_Client_Type *_type, Network_Node *node)
{
    Read(u8, type, node);
    *_type = (MS_Client_Type)type;
    return true;
}

bool write_MS_Client_Type(MS_Client_Type type, Network_Node *node)
{
    Write(u8, type, node);
    return true;
}


bool read_MSB_Packet_Header(MSB_Packet_Header *_header, Network_Node *node)
{    
    Zero(*_header);
    Read_To_Ptr(MSB_Packet_Type, &_header->type, node);

    switch(_header->type)
    {
        case MSB_GOODBYE: {} break;

        case MSB_HELLO: {
            auto *p = &_header->hello;
            Read_To_Ptr(User_ID, &p->user, node);
            Read_To_Ptr(MS_Client_Type, &p->client_type, node);
        } break;

        case MSB_PLACE_ORDER: {
            auto *p = &_header->place_order;
            Read_To_Ptr(Item_Type_ID, &p->item_type, node);
            Read_To_Ptr(Money,        &p->price, node);
            Read_To_Ptr(bool,         &p->is_buy_order, node);
        } break;

        default: Assert(false); break;
    }
    
    return true;
}



bool enqueue_MSB_HELLO_packet(Network_Node *node, User_ID user, MS_Client_Type client_type)
{
    begin_outbound_packet(node);
    {
        Write(MSB_Packet_Type, MSB_HELLO, node);
        //--

        Write(User_ID, user, node);
        Write(MS_Client_Type, client_type, node);
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_MSB_GOODBYE_packet(Network_Node *node)
{
    begin_outbound_packet(node);
    {
        Write(MSB_Packet_Type, MSB_GOODBYE, node);
        //--
    }
    end_outbound_packet(node);
    return true;
}

bool send_MSB_GOODBYE_packet_now(Network_Node *node)
{
    Send_Now_NoArgs(MSB_GOODBYE, node);
    return true;
}


bool enqueue_MSB_PLACE_ORDER_packet(Network_Node *node, Item_Type_ID item_type, Money price, bool is_buy_order)
{
    begin_outbound_packet(node);
    {
        Write(MSB_Packet_Type, MSB_PLACE_ORDER, node);
        //--
        
        Write(Item_Type_ID, item_type, node);
        Write(Money,        price, node);
        Write(bool,         is_buy_order, node);
    }
    end_outbound_packet(node);
    return true;
}

bool send_MSB_PLACE_ORDER_packet_now(Network_Node *node, Item_Type_ID item_type, Money price, bool is_buy_order)
{
    Send_Now(MSB_PLACE_ORDER, node, item_type, price, is_buy_order);
    return true;
}



#endif // MARKET_SERVER_BOUND_INCLUDED
