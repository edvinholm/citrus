
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

    UCB_TRANSACTION_MESSAGE = 5,
    UCB_ITEM_INFO = 6
};

struct UCB_Transaction_Commit_Vote_Payload
{
    union {
        Item item;
    };
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
            String username;
            v4 color;
            Money money;
        } user_init;

        struct {
            User_ID id;
            String username;
            v4 color;
            Money money;
        } user_update;

        struct {
            US_Transaction_Type transaction_type;
            Transaction_Message message;

            union {
                UCB_Transaction_Commit_Vote_Payload commit_vote_payload; // Only valid if message == TRANSACTION_VOTE_COMMIT
            };
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


bool enqueue_UCB_USER_INIT_packet(Network_Node *node, User_ID id, String username, v4 color, Money money, Item *inventory)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_USER_INIT, node);
        //--
        
        Write(User_ID, id,       node);
        Write(String,  username, node);
        Write(v4,      color,    node);
        Write(Money,   money,    node);
        for(int i = 0; i < ARRLEN(S__User::inventory); i++) {
            Write(Item, inventory[i], node);
        }
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_UCB_USER_UPDATE_packet(Network_Node *node, User_ID id, String username, v4 color, Money money, Item *inventory)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_USER_UPDATE, node);
        //--
        
        Write(User_ID, id,       node);
        Write(String,  username, node);
        Write(v4,      color,    node);
        Write(Money,   money,    node);
        for(int i = 0; i < ARRLEN(S__User::inventory); i++) {
            Write(Item, inventory[i], node);
        }
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_UCB_TRANSACTION_MESSAGE_packet(Network_Node *node,
                                            US_Transaction_Type transaction_type,
                                            Transaction_Message message,
                                            UCB_Transaction_Commit_Vote_Payload *commit_vote_payload = NULL)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_TRANSACTION_MESSAGE, node);
        //--

        Write(US_Transaction_Type, transaction_type, node);
        Write(Transaction_Message, message, node);

        if(message == TRANSACTION_VOTE_COMMIT)
        {
            Assert(commit_vote_payload);
            switch(transaction_type)
            {
                case US_T_ITEM: {
                    Write(Item, commit_vote_payload->item, node);
                } break;
            }
        }
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
            Read_To_Ptr(Money,   &p->money,    node);
        } break;

        case UCB_USER_UPDATE: {
            auto *p = &_header->user_update;
            Read_To_Ptr(User_ID, &p->id,       node);
            Read_To_Ptr(String,  &p->username, node);
            Read_To_Ptr(v4,      &p->color,    node);
            Read_To_Ptr(Money,   &p->money,    node);
        } break;
            
        case UCB_TRANSACTION_MESSAGE: {
            auto *p = &_header->transaction_message;
            Read_To_Ptr(US_Transaction_Type, &p->transaction_type, node);
            Read_To_Ptr(Transaction_Message, &p->message, node);

            if(p->message == TRANSACTION_VOTE_COMMIT)
            {
                switch(p->transaction_type)
                {
                    case US_T_ITEM: {
                        Read_To_Ptr(Item, &p->commit_vote_payload.item, node);
                    } break;
                }
            }
        } break;

        default: Assert(false); return false;
    }

    return true;
}


#endif
