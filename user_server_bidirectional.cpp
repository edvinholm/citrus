



enum US_Transaction_Type
{
    US_T_ITEM = 1
};


struct US_Transaction
{
    US_Transaction_Type type;
    union {
        struct {
            bool is_server_bound; // If true, the item should be transferred client -> server, otherwise server -> client
            union {
                struct {
                    Item item;
                } server_bound;
                
                struct {
                    Item_ID item_id;      
                } client_bound;
            };
        } item_details;
    };
};


bool write_US_Transaction_Type(US_Transaction_Type type, Network_Node *node)
{
    Write(u16, type, node);
    return true;
}

// @Norelease TODO: Check that it is a valid type.
bool read_US_Transaction_Type(US_Transaction_Type *_type, Network_Node *node)
{
    Read(u16, type, node);
    *_type = (US_Transaction_Type)type;
    return true;
}

bool read_US_Transaction(US_Transaction *_transaction, Network_Node *node)
{
    Read_To_Ptr(US_Transaction_Type, &_transaction->type, node);
    switch(_transaction->type) {
        
        case US_T_ITEM: {
            auto *x = &_transaction->item_details;
            
            Read_To_Ptr(bool,    &x->is_server_bound, node);
            if(x->is_server_bound) {
                Read_To_Ptr(Item, &x->server_bound.item, node);
            } else {
                Read_To_Ptr(Item_ID, &x->client_bound.item_id, node);
            }
        } break;

        default: Assert(false); return false;
    }

    return true;
}

bool write_US_Transaction(US_Transaction transaction, Network_Node *node)
{
    Write(US_Transaction_Type, transaction.type, node);
    
    switch(transaction.type)
    {
        case US_T_ITEM: {
            auto *x = &transaction.item_details;
            
            Write(bool,    x->is_server_bound, node);
            if(x->is_server_bound) {
                Write(Item, x->server_bound.item, node);
            } else {
                Write(Item_ID, x->client_bound.item_id, node);
            }
        } break;

        default: Assert(false); return false;
    }

    return true;
}

