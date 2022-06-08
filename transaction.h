
enum Node_Type;

typedef u32 Transaction_ID; // This is local to the transaction initiator.

enum Transaction_Message
{
    TRANSACTION_PREPARE,
    
    TRANSACTION_VOTE_COMMIT,
    TRANSACTION_VOTE_ABORT,

    TRANSACTION_COMMAND_COMMIT,
    TRANSACTION_COMMAND_ABORT
};



// NOTE: This is the structure we use for queued and active transactions.
//       It is not sent over the network -- we use it to locally keep
//       track of ongoing transactions. Used by all node types.
struct Transaction
{
    Transaction_ID id;
    bool incoming; // Did we initiate the Transaction (false) or did another node request it (true)?

    Node_Type other_node_type;
    union {
        US_Transaction us; // if other_node_type == NODE_USER.
    };
};
