
#ifndef USER_CLIENT_BOUND_INCLUDED
#define USER_CLIENT_BOUND_INCLUDED



// IMPORTANT: Must fit in a u64
enum User_Connect_Status
{
    USER_CONNECT__CONNECTED              = 1,
    USER_CONNECT__INCORRECT_CREDENTIALS  = 2
};

enum UCB_Packet_Type
{
    UCB_HELLO   = 1,
    UCB_GOODBYE = 2,
    
    UCB_USER_INIT = 3,
    UCB_USER_UPDATE = 4,

    UCB_TRANSACTION_MESSAGE = 5
};

struct UCB_Packet_Header
{
    UCB_Packet_Type type;
    union {

        struct {
            User_Connect_Status connect_status;
        } hello;
        
        struct {
            User_ID id;
            String  username;
            v4      color;
        } user_init;

        struct {
            User_ID id;
            String  username;
            v4      color;
        } user_update;

        struct {
            Transaction_Message message;
        } transaction_message;
    };
};

struct UCB_Transaction_Message
{
    // @Norelease: This should contain an ID (probably)
    
    Transaction_Message message;
};

inline
bool read_UCB_Packet_Type(UCB_Packet_Type *_type, Network_Node *node)
{
    u64 i;
    if(!read_u64(&i, node)) return false;
    *_type = (UCB_Packet_Type)i;
    return true;
}

inline
bool write_UCB_Packet_Type(UCB_Packet_Type type, Network_Node *node)
{
    return write_u64(type, node);
}


bool read_User_Connect_Status(User_Connect_Status *_status, Network_Node *node)
{
    Read(u8, status, node);
    *_status = (User_Connect_Status)status;
    return true;
}

bool write_User_Connect_Status(User_Connect_Status status, Network_Node *node)
{
    Write(u8, status, node);
    return true;
}

bool enqueue_UCB_HELLO_packet(Network_Node *node, User_Connect_Status connect_status)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_HELLO, node);
        //--

        Write(User_Connect_Status, connect_status, node);
    }
    end_outbound_packet(node);
    return true;
}

bool send_UCB_HELLO_packet_now(Network_Node *node, User_Connect_Status connect_status)
{
    Send_Now(UCB_HELLO, node, connect_status);
    return true;
}


bool enqueue_UCB_GOODBYE_packet(Network_Node *node)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_GOODBYE, node);
        //--
    }
    end_outbound_packet(node);
    return true;
}

bool send_UCB_GOODBYE_packet_now(Network_Node *node)
{
    Send_Now_NoArgs(UCB_GOODBYE, node);
    return true;
}


bool enqueue_UCB_USER_INIT_packet(Network_Node *node, User_ID id, String username, v4 color, Item *inventory)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_USER_INIT, node);
        //--
        
        Write(User_ID, id,       node);
        Write(String,  username, node);
        Write(v4,      color,    node);
        for(int i = 0; i < ARRLEN(S__User::inventory); i++) {
            Write(Item, inventory[i], node);
        }
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_UCB_USER_UPDATE_packet(Network_Node *node, User_ID id, String username, v4 color, Item *inventory)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_USER_UPDATE, node);
        //--
        
        Write(User_ID, id,       node);
        Write(String,  username, node);
        Write(v4,      color,    node);
        for(int i = 0; i < ARRLEN(S__User::inventory); i++) {
            Write(Item, inventory[i], node);
        }
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_UCB_TRANSACTION_MESSAGE_packet(Network_Node *node, Transaction_Message message)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_TRANSACTION_MESSAGE, node);
        //--

        Write(Transaction_Message, message, node);
    }
    end_outbound_packet(node);
    return true;
}




bool read_UCB_Packet_Header(UCB_Packet_Header *_header, Network_Node *node)
{
    // @Jai: Auto-generate.
    
    Read_To_Ptr(UCB_Packet_Type, &_header->type, node);
    //--
    
    switch(_header->type) {
        case UCB_HELLO: {
            auto *p = &_header->hello;
            Read_To_Ptr(User_Connect_Status, &p->connect_status, node);
        } break;
            
        case UCB_GOODBYE: {
        } break;

        case UCB_USER_INIT: {
            auto *p = &_header->user_init;
            Read_To_Ptr(User_ID, &p->id,       node);
            Read_To_Ptr(String,  &p->username, node);
            Read_To_Ptr(v4,      &p->color,    node);
        } break;

        case UCB_USER_UPDATE: {
            auto *p = &_header->user_update;
            Read_To_Ptr(User_ID, &p->id,       node);
            Read_To_Ptr(String,  &p->username, node);
            Read_To_Ptr(v4,      &p->color,    node);
        } break;
            
        case UCB_TRANSACTION_MESSAGE: {
            auto *p = &_header->transaction_message;
            Read_To_Ptr(Transaction_Message, &p->message, node);
        } break;

        default: Assert(false); return false;
    }

    return true;
}


#endif
