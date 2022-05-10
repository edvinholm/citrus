
inline
bool read_MCB_Packet_Type(MCB_Packet_Type *_type, Network_Node *node)
{
    u64 i;
    if(!read_u64(&i, node)) return false;
    *_type = (MCB_Packet_Type)i;
    return true;
}

inline
bool write_MCB_Packet_Type(MCB_Packet_Type type, Network_Node *node)
{
    return write_u64(type, node);
}


bool read_Market_Connect_Status(Market_Connect_Status *_status, Network_Node *node)
{
    Read(u8, status, node);
    *_status = (Market_Connect_Status)status;
    return true;
}

bool write_Market_Connect_Status(Market_Connect_Status status, Network_Node *node)
{
    Write(u8, status, node);
    return true;
}

bool enqueue_MCB_HELLO_packet(Network_Node *node, Market_Connect_Status connect_status)
{
    begin_outbound_packet(node);
    {
        Write(MCB_Packet_Type, MCB_HELLO, node);
        //--

        Write(Market_Connect_Status, connect_status, node);
    }
    end_outbound_packet(node);
    return true;
}

bool send_MCB_HELLO_packet_now(Network_Node *node, Market_Connect_Status connect_status)
{
    Send_Now(MCB_HELLO, node, connect_status);
    return true;
}


bool enqueue_MCB_GOODBYE_packet(Network_Node *node)
{
    begin_outbound_packet(node);
    {
        Write(MCB_Packet_Type, MCB_GOODBYE, node);
        //--
    }
    end_outbound_packet(node);
    return true;
}

bool send_MCB_GOODBYE_packet_now(Network_Node *node)
{
    Send_Now_NoArgs(MCB_GOODBYE, node);
    return true;
}


bool enqueue_MCB_MARKET_INIT_packet(Network_Node *node)
{
    begin_outbound_packet(node);
    {
        Write(MCB_Packet_Type, MCB_MARKET_INIT, node);
        //--
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_MCB_MARKET_UPDATE_packet(Network_Node *node, S__Market_View *view)
{
    begin_outbound_packet(node);
    {
        Write(MCB_Packet_Type, MCB_MARKET_UPDATE, node);
        //--

        Write(Market_View, view, node);
    }
    end_outbound_packet(node);
    return true;
}




bool read_MCB_Packet_Header(MCB_Packet_Header *_header, Network_Node *node)
{
    // @Jai: Auto-generate.
    
    Read_To_Ptr(MCB_Packet_Type, &_header->type, node);
    //--
    
    switch(_header->type) {
        case MCB_HELLO: {
            auto *p = &_header->hello;
            Read_To_Ptr(Market_Connect_Status, &p->connect_status, node);
        } break;
            
        case MCB_GOODBYE: {
        } break;

        case MCB_MARKET_INIT: {
            auto *p = &_header->market_init;
        } break;

        case MCB_MARKET_UPDATE: {
            auto *p = &_header->market_update;

            Read_To_Ptr(Market_View, &p->view, node);
        } break;

        default: Assert(false); return false;
    }

    return true;
}
