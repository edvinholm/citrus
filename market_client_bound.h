
enum Market_Connect_Status: u8
{
    MARKET_CONNECT__CONNECTED              = 1,
    MARKET_CONNECT__INCORRECT_CREDENTIALS  = 2
};

enum MCB_Packet_Type
{
    MCB_HELLO   = 1,
    MCB_GOODBYE = 2,
    
    MCB_MARKET_INIT = 3,
    MCB_MARKET_UPDATE = 4
};


struct MCB_Packet_Header
{
    MCB_Packet_Type type;
    union {

        struct {
            Market_Connect_Status connect_status;
        } hello;
        
        struct {
        } market_init;

        struct {
            S__Market_View view;
        } market_update;

    };
};

