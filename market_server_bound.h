
enum MS_Client_Type
{
    MS_CLIENT_PLAYER = 1
};

enum MSB_Packet_Type
{
    MSB_HELLO = 1,
    MSB_GOODBYE = 2,

    MSB_PLACE_ORDER = 3,
    MSB_SET_VIEW_TARGET = 4
};

// User Server Bound Packet Header
struct MSB_Packet_Header
{
    MSB_Packet_Type type;
    union {
        struct {
            User_ID user;
            MS_Client_Type client_type;
        } hello;
        
        struct {
        } goodbye;

        struct {
            Money        price;
            bool         is_buy_order; // If false, it is a sell order.

            union {
                struct {
                    Item_Type_ID item_type;
                } buy;
                
                struct {
                    Item_ID item_id;
                } sell;
            };
            
        } place_order;

        struct {
            Market_View_Target target;
        } set_view_target;
    };
};


