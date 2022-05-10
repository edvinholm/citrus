

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

