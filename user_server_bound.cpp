
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

enum USB_Transaction_Type
{
    USB_T_ITEM = 1
};

struct USB_Transaction
{
    USB_Transaction_Type type;
    union {
        struct {
            Item item;
        } item_details;
    };
};

enum US_Client_Type
{
    US_CLIENT_PLAYER = 1,
    US_CLIENT_RS = 2
};

// User Server Bound Packet Header
struct USB_Packet_Header
{
    USB_Packet_Type type;
    union {
        struct {
            User_ID user;
            US_Client_Type client_type;
        } hello;
        
        struct {
        } goodbye;
        
        struct {
            Transaction_Message message;
            USB_Transaction transaction;
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



bool write_USB_Transaction_Type(USB_Transaction_Type type, Network_Node *node)
{
    Write(u16, type, node);
    return true;
}

// @Norelease TODO: Check that it is a valid type.
bool read_USB_Transaction_Type(USB_Transaction_Type *_type, Network_Node *node)
{
    Read(u16, type, node);
    *_type = (USB_Transaction_Type)type;
    return true;
}

bool read_USB_Transaction(USB_Transaction *_transaction, Network_Node *node)
{
    Read_To_Ptr(USB_Transaction_Type, &_transaction->type, node);
    switch(_transaction->type) {
        case USB_T_ITEM: {
            auto *x = &_transaction->item_details;
            Read_To_Ptr(Item, &x->item, node);
        } break;

        default: Assert(false); return false;
    }

    return true;
}

bool write_USB_Transaction(USB_Transaction transaction, Network_Node *node)
{
    Write(USB_Transaction_Type, transaction.type, node);
    
    switch(transaction.type)
    {
        case USB_T_ITEM: {
            Write(Item, transaction.item_details.item, node);
        } break;

        default: Assert(false); return false;
    }

    return true;
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
        } break;

        case USB_TRANSACTION_MESSAGE: {
            auto *p = &_header->transaction_message;
            Read_To_Ptr(Transaction_Message, &p->message, node);
            Read_To_Ptr(USB_Transaction, &p->transaction, node);
        } break;

        default: Assert(false); break;
    }
    
    return true;
}



bool enqueue_USB_HELLO_packet(Network_Node *node, User_ID user, US_Client_Type client_type)
{
    begin_outbound_packet(node);
    {
        Write(USB_Packet_Type, USB_HELLO, node);
        //--

        Write(User_ID, user, node);
        Write(US_Client_Type, client_type, node);
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


bool enqueue_USB_TRANSACTION_MESSAGE_packet(Network_Node *node, Transaction_Message message, USB_Transaction transaction)
{
    begin_outbound_packet(node);
    {
        Write(USB_Packet_Type, USB_TRANSACTION_MESSAGE, node);
        //--
    
        Write(Transaction_Message, message,    node);
        Write(USB_Transaction,     transaction, node);
    }
    end_outbound_packet(node);
    return true;
}



#endif // USER_SERVER_BOUND_INCLUDED
