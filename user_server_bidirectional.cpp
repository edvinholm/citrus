

const int MAX_US_TRANSACTION_OPERATIONS = 4; // Max number of operations per transaction.
static_assert(MAX_US_TRANSACTION_OPERATIONS >= 1);
static_assert(MAX_US_TRANSACTION_OPERATIONS <= U8_MAX);

enum US_Transaction_Operation_Type
{
    US_T_ITEM_TRANSFER      = 1,
    
    US_T_ITEM_RESERVE   = 2,
    US_T_ITEM_UNRESERVE = 3,
    
    US_T_SLOT_RESERVE   = 4, // NOTE: This is to reserve an EMPTY inventory slot.
    US_T_SLOT_UNRESERVE = 5,


    US_T_MONEY_TRANSFER  = 6,
    
    US_T_MONEY_RESERVE   = 7,
    US_T_MONEY_UNRESERVE = 8
};

struct US_Transaction_Operation
{
    US_Transaction_Operation_Type type;
    
    union {
        struct {
            bool is_server_bound; // If true, the item should be transferred client -> server, otherwise server -> client
            union {
                struct {
                    Item item;
                    u32 slot_ix_plus_one; // If this is zero, the server can place the item wherever it wants.
                } server_bound;
               
                struct {
                    Item_ID item_id;      
                } client_bound;
            };
        } item_transfer;

        struct {
            Item_ID item_id;
        } item_reserve;
        
        struct {
            Item_ID item_id;
        } item_unreserve;

        
        struct {
        } slot_reserve;
        
        struct {
            u32 slot_ix;
        } slot_unreserve;


        struct {
            Money amount;
            bool do_unreserve;
        } money_transfer;

        struct {
            Money amount;
        } money_reserve;

        struct {
            Money amount;
        } money_unreserve;
    };
};

struct US_Transaction
{
    u8 num_operations;
    US_Transaction_Operation operations[MAX_US_TRANSACTION_OPERATIONS];
};


bool write_US_Transaction_Operation_Type(US_Transaction_Operation_Type type, Network_Node *node)
{
    Write(u16, type, node);
    return true;
}

// @Norelease TODO: Check that it is a valid type.
bool read_US_Transaction_Operation_Type(US_Transaction_Operation_Type *_type, Network_Node *node)
{
    Read(u16, type, node);
    *_type = (US_Transaction_Operation_Type)type;
    return true;
}

bool read_US_Transaction_Operation(US_Transaction_Operation *_operation, Network_Node *node)
{
    Read_To_Ptr(US_Transaction_Operation_Type, &_operation->type, node);
    
    switch(_operation->type) {
        
        case US_T_ITEM_TRANSFER: {
            auto *x = &_operation->item_transfer;
            
            Read_To_Ptr(bool,    &x->is_server_bound, node);
            if(x->is_server_bound) {
                Read_To_Ptr(Item, &x->server_bound.item, node);
                Read_To_Ptr(u32,  &x->server_bound.slot_ix_plus_one, node);
            } else {
                Read_To_Ptr(Item_ID, &x->client_bound.item_id, node);
            }
        } break;

        case US_T_ITEM_RESERVE: {
            auto *x = &_operation->item_reserve;

            Read_To_Ptr(Item_ID, &x->item_id, node);
        } break;
            
        case US_T_ITEM_UNRESERVE: {
            auto *x = &_operation->item_unreserve;

            Read_To_Ptr(Item_ID, &x->item_id, node);            
        } break;
            
        case US_T_SLOT_RESERVE: {
            auto *x = &_operation->slot_reserve;

            // No parameters here yet.
        } break;
            
        case US_T_SLOT_UNRESERVE: {
            auto *x = &_operation->slot_unreserve;

            Read_To_Ptr(u32, &x->slot_ix, node); // @Norelease: Check that the slot index is in range.
        } break;

            
        case US_T_MONEY_TRANSFER: {
            auto *x = &_operation->money_transfer;

            Read_To_Ptr(Money, &x->amount, node);
            Read_To_Ptr(bool,  &x->do_unreserve, node);
        } break;
            
        case US_T_MONEY_RESERVE: {
            auto *x = &_operation->money_reserve;

            Read_To_Ptr(Money, &x->amount, node);
        } break;
            
        case US_T_MONEY_UNRESERVE: {
            auto *x = &_operation->money_unreserve;

            Read_To_Ptr(Money, &x->amount, node);
        } break;
            

        default: Assert(false); return false;
    }

    return true;
}

bool write_US_Transaction_Operation(US_Transaction_Operation operation, Network_Node *node)
{
    Write(US_Transaction_Operation_Type, operation.type, node);
    
    switch(operation.type) {
        
        case US_T_ITEM_TRANSFER: {
            auto *x = &operation.item_transfer;
            
            Write(bool,    x->is_server_bound, node);
            if(x->is_server_bound) {
                Write(Item, x->server_bound.item, node);
                Write(u32,  x->server_bound.slot_ix_plus_one, node);
            } else {
                Write(Item_ID, x->client_bound.item_id, node);
            }
        } break;

        case US_T_ITEM_RESERVE: {
            auto *x = &operation.item_reserve;

            Write(Item_ID, x->item_id, node);
        } break;
            
        case US_T_ITEM_UNRESERVE: {
            auto *x = &operation.item_unreserve;

            Write(Item_ID, x->item_id, node);            
        } break;
            
        case US_T_SLOT_RESERVE: {
            auto *x = &operation.slot_reserve;

            // No parameters here yet.
        } break;
            
        case US_T_SLOT_UNRESERVE: {
            auto *x = &operation.slot_unreserve;

            Write(u32, x->slot_ix, node);
        } break;

            
        case US_T_MONEY_TRANSFER: {
            auto *x = &operation.money_transfer;

            Write(Money, x->amount, node);
            Write(bool,  x->do_unreserve, node);
        } break;
            
        case US_T_MONEY_RESERVE: {
            auto *x = &operation.money_reserve;

            Write(Money, x->amount, node);
        } break;
            
        case US_T_MONEY_UNRESERVE: {
            auto *x = &operation.money_unreserve;

            Write(Money, x->amount, node);
        } break;

            
        default: Assert(false); return false;
    }

    return true;
}


bool read_US_Transaction(US_Transaction *_transaction, Network_Node *node)
{
    Read_To_Ptr(u8, &_transaction->num_operations, node);
    Fail_If_True(_transaction->num_operations <= 0);
    Fail_If_True(_transaction->num_operations > ARRLEN(_transaction->operations));

    for(int i = 0; i < _transaction->num_operations; i++)
    {
        Assert(i < ARRLEN(_transaction->operations));
        Read_To_Ptr(US_Transaction_Operation, &_transaction->operations[i], node);
    }

    return true;
}

bool write_US_Transaction(US_Transaction *transaction, Network_Node *node)
{
    Fail_If_True(transaction->num_operations <= 0);
    Fail_If_True(transaction->num_operations > ARRLEN(transaction->operations));
    
    Write(u8, transaction->num_operations, node);

    for(int i = 0; i < transaction->num_operations; i++)
    {
        Assert(i < ARRLEN(transaction->operations));
        Write(US_Transaction_Operation, transaction->operations[i], node);
    }
    
    return true;
}
