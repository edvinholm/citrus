
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
