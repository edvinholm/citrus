
#ifndef USER_SERVER_BOUND_INCLUDED
#define USER_SERVER_BOUND_INCLUDED




#define Write_USB_Header(Packet_Type, Node_Ptr)    \
    Fail_If_True(!write_USB_Packet_Header({Packet_Type}, Node_Ptr))


enum USB_Packet_Type
{
    USB_HELLO = 1,
    USB_GOODBYE = 2,

    USB_TRANSACTION_MESSAGE = 3
};

// User Server Bound Packet Header
struct USB_Packet_Header
{
    USB_Packet_Type type;
    union {
        struct {
            User_ID user;
            US_Client_Type client_type;

            union {
                struct {
                    u32 server_id;
                } non_player;
            };
        } hello;
        
        struct {
        } goodbye;
        
        struct {
            Transaction_Message message;
            US_Transaction transaction;
        } transaction_message;
    };
};


bool read_USB_Packet_Type(USB_Packet_Type *_type, Network_Node *node)
{
    u64 i;
    if(!read_u64(&i, node)) return false;
    *_type = (USB_Packet_Type)i;
    return true;
}


bool write_USB_Packet_Type(USB_Packet_Type type, Network_Node *node)
{
    return write_u64(type, node);
}



bool read_US_Client_Type(US_Client_Type *_type, Network_Node *node)
{
    Read(u8, type, node);
    *_type = (US_Client_Type)type;
    return true;
}

bool write_US_Client_Type(US_Client_Type type, Network_Node *node)
{
    Write(u8, type, node);
    return true;
}


bool read_USB_Packet_Header(USB_Packet_Header *_header, Network_Node *node)
{    
    Zero(*_header);
    Read_To_Ptr(USB_Packet_Type, &_header->type, node);

    switch(_header->type)
    {
        case USB_GOODBYE: {} break;

        case USB_HELLO: {
            auto *p = &_header->hello;
            Read_To_Ptr(User_ID, &p->user, node);
            Read_To_Ptr(US_Client_Type, &p->client_type, node);

            if(p->client_type != US_CLIENT_PLAYER) {
                Read_To_Ptr(u32, &p->non_player.server_id, node);
            }
        } break;

        case USB_TRANSACTION_MESSAGE: {
            auto *p = &_header->transaction_message;
            Read_To_Ptr(Transaction_Message, &p->message, node);

            if(p->message == TRANSACTION_PREPARE) {
                Read_To_Ptr(US_Transaction, &p->transaction, node);
            }
        } break;

        default: Assert(false); break;
    }
    
    return true;
}



// NOTE: Pass zero for server_id if client_type == US_CLIENT_PLAYER.
bool enqueue_USB_HELLO_packet(Network_Node *node, User_ID user, US_Client_Type client_type, u32 client_node_id)
{
    Assert(client_type == US_CLIENT_PLAYER || client_node_id > 0);
    
    begin_outbound_packet(node);
    {
        Write(USB_Packet_Type, USB_HELLO, node);
        //--

        Write(User_ID, user, node);
        Write(US_Client_Type, client_type, node);
        
        if(client_type != US_CLIENT_PLAYER) {
            Write(u32, client_node_id, node);
        }
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_USB_GOODBYE_packet(Network_Node *node)
{
    begin_outbound_packet(node);
    {
        Write(USB_Packet_Type, USB_GOODBYE, node);
        //--
    }
    end_outbound_packet(node);
    return true;
}

bool send_USB_GOODBYE_packet_now(Network_Node *node)
{
    Send_Now_NoArgs(USB_GOODBYE, node);
    return true;
}


bool enqueue_USB_TRANSACTION_MESSAGE_packet(Network_Node *node, Transaction_Message message, US_Transaction *transaction)
{
    if(message == TRANSACTION_PREPARE) {
        Assert(transaction);
    } else {
        Assert(!transaction);
    }
    
    begin_outbound_packet(node);
    {
        Write(USB_Packet_Type, USB_TRANSACTION_MESSAGE, node);
        //--
    
        Write(Transaction_Message, message,    node);

        if(message == TRANSACTION_PREPARE) {
            Write(US_Transaction, transaction, node);
        }
    }
    end_outbound_packet(node);
    return true;
}




#endif // USER_SERVER_BOUND_INCLUDED
