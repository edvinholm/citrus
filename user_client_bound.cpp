
#ifndef USER_CLIENT_BOUND_INCLUDED
#define USER_CLIENT_BOUND_INCLUDED

enum User_Connect_Status: u64 // @Cleanup: This does not need to be 64 bits (Change this here and in network code)
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

struct UCB_Transaction_Commit_Vote_Payload
{
    u8 num_operations; // NOTE: This is just so that we know how to read the header. Client should know what operations it requested.
    US_Transaction_Operation_Type operation_types[MAX_US_TRANSACTION_OPERATIONS]; // NOTE: This is just so that we know how to read the header. Client should know what operations it requested.

    struct Payload {
        union {
            struct {
                Item item;  
            } item_transfer;
    
            struct {
                Item item;
            } item_reserve;
    
            struct {
            } item_unreserve;
    
            struct {
                u32 slot_ix;  
            } slot_reserve;
    
            struct {
            } slot_unreserve;
        };
    };

    Payload operation_payloads[MAX_US_TRANSACTION_OPERATIONS];
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
            Money reserved_money;
        } user_init;

        struct {
            User_ID id;
            String username;
            v4 color;
            
            Money money;
            Money reserved_money;
        } user_update;

        struct {
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


bool read_UCB_Transaction_Commit_Vote_Payload(UCB_Transaction_Commit_Vote_Payload *_payload, Network_Node *node)
{
    Read_To_Ptr(u8, &_payload->num_operations, node);
    Fail_If_True(_payload->num_operations <= 0);
    Fail_If_True(_payload->num_operations > MAX_US_TRANSACTION_OPERATIONS);

    Assert(_payload->num_operations <= ARRLEN(_payload->operation_types));
    for(int i = 0; i < _payload->num_operations; i++) {
        Read_To_Ptr(US_Transaction_Operation_Type, &_payload->operation_types[i], node);
    }

    for(int i = 0; i < _payload->num_operations; i++) {
        auto *p = &_payload->operation_payloads[i];
        
        switch(_payload->operation_types[i]) // @Jai: #complete
        {
            case US_T_ITEM_TRANSFER: {
                auto *x = &p->item_transfer;
                Read_To_Ptr(Item, &x->item, node);
            } break;

                
            case US_T_ITEM_RESERVE: {
                auto *x = &p->item_reserve;
                Read_To_Ptr(Item, &x->item, node);
            } break;
                
            case US_T_ITEM_UNRESERVE: {
                auto *x = &p->item_unreserve;
                
                // nothing here yet.
            } break;

                
            case US_T_SLOT_RESERVE: {
                auto *x = &p->slot_reserve;
                Read_To_Ptr(u32, &x->slot_ix, node);
            } break;
                
            case US_T_SLOT_UNRESERVE: {
                auto *x = &p->slot_unreserve;
                
                // nothing here yet.
            } break;
        }
    }

    return true;
}

bool write_UCB_Transaction_Commit_Vote_Payload(UCB_Transaction_Commit_Vote_Payload *payload, Network_Node *node)
{
    Assert(payload->num_operations <= ARRLEN(payload->operation_types));
    
    Fail_If_True(payload->num_operations <= 0);
    Fail_If_True(payload->num_operations > MAX_US_TRANSACTION_OPERATIONS);
    Write(u8, payload->num_operations, node);

    for(int i = 0; i < payload->num_operations; i++) {
        Write(US_Transaction_Operation_Type, payload->operation_types[i], node);
    }

    for(int i = 0; i < payload->num_operations; i++) {
        auto *p = &payload->operation_payloads[i];
        
        switch(payload->operation_types[i]) // @Jai: #complete
        {
            case US_T_ITEM_TRANSFER: {
                auto *x = &p->item_transfer;
                Write(Item, x->item, node);
            } break;

                
            case US_T_ITEM_RESERVE: {
                auto *x = &p->item_reserve;
                Write(Item, x->item, node);
            } break;
                
            case US_T_ITEM_UNRESERVE: {
                auto *x = &p->item_unreserve;
                
                // nothing here yet.
            } break;

                
            case US_T_SLOT_RESERVE: {
                auto *x = &p->slot_reserve;
                Write(u32, x->slot_ix, node);
            } break;
                
            case US_T_SLOT_UNRESERVE: {
                auto *x = &p->slot_unreserve;
                
                // nothing here yet.
            } break;
        }
    }

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


bool enqueue_UCB_USER_INIT_packet(Network_Node *node, User_ID id, String username, v4 color, Money money, Money reserved_money, Inventory_Slot *inventory)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_USER_INIT, node);
        //--
        
        Write(User_ID, id,       node);
        Write(String,  username, node);
        Write(v4,      color,    node);
        
        Write(Money,   money,    node);
        Write(Money,   reserved_money, node);
        
        for(int i = 0; i < ARRLEN(S__User::inventory); i++) {
            Write(Inventory_Slot, inventory + i, node);
        }
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_UCB_USER_UPDATE_packet(Network_Node *node, User_ID id, String username, v4 color, Money money, Money reserved_money, Inventory_Slot *inventory)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_USER_UPDATE, node);
        //--
        
        Write(User_ID, id,       node);
        Write(String,  username, node);
        Write(v4,      color,    node);
        
        Write(Money,   money,    node);
        Write(Money,   reserved_money, node);
        
        for(int i = 0; i < ARRLEN(S__User::inventory); i++) {
            Write(Inventory_Slot, inventory + i, node);
        }
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_UCB_TRANSACTION_MESSAGE_packet(Network_Node *node,
                                            Transaction_Message message,
                                            UCB_Transaction_Commit_Vote_Payload *commit_vote_payload = NULL)
{
    begin_outbound_packet(node);
    {
        Write(UCB_Packet_Type, UCB_TRANSACTION_MESSAGE, node);
        //--

        Write(Transaction_Message, message, node);

        if(message == TRANSACTION_VOTE_COMMIT)
        {
            Assert(commit_vote_payload);
            Write(UCB_Transaction_Commit_Vote_Payload, commit_vote_payload, node);
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
            Read_To_Ptr(Money,   &p->reserved_money, node);
        } break;

        case UCB_USER_UPDATE: {
            auto *p = &_header->user_update;
            Read_To_Ptr(User_ID, &p->id,       node);
            Read_To_Ptr(String,  &p->username, node);
            Read_To_Ptr(v4,      &p->color,    node);
            
            Read_To_Ptr(Money,   &p->money,    node);
            Read_To_Ptr(Money,   &p->reserved_money, node);
        } break;
            
        case UCB_TRANSACTION_MESSAGE: {
            auto *p = &_header->transaction_message;
            Read_To_Ptr(Transaction_Message, &p->message, node);

            if(p->message == TRANSACTION_VOTE_COMMIT)
            {
                Read_To_Ptr(UCB_Transaction_Commit_Vote_Payload, &p->commit_vote_payload, node);
            }
        } break;

        default: Assert(false); return false;
    }

    return true;
}


#endif
