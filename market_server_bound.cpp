
#ifndef MARKET_SERVER_BOUND_INCLUDED
#define MARKET_SERVER_BOUND_INCLUDED


enum MSB_Packet_Type
{
    MSB_HELLO = 1,
    MSB_GOODBYE = 2,

    MSB_PLACE_ORDER = 3,
    MSB_SET_WATCHED_ARTICLE = 4
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
            Item_Type_ID article;
        } set_watched_article;
    };
};


bool read_MSB_Packet_Type(MSB_Packet_Type *_type, Network_Node *node)
{
    u64 i;
    if(!read_u64(&i, node)) return false;
    *_type = (MSB_Packet_Type)i;
    return true;
}


bool write_MSB_Packet_Type(MSB_Packet_Type type, Network_Node *node)
{
    return write_u64(type, node);
}



bool read_MS_Client_Type(MS_Client_Type *_type, Network_Node *node)
{
    Read(u8, type, node);
    *_type = (MS_Client_Type)type;
    return true;
}

bool write_MS_Client_Type(MS_Client_Type type, Network_Node *node)
{
    Write(u8, type, node);
    return true;
}


bool read_MSB_Packet_Header(MSB_Packet_Header *_header, Network_Node *node)
{    
    Zero(*_header);
    Read_To_Ptr(MSB_Packet_Type, &_header->type, node);

    switch(_header->type)
    {
        case MSB_GOODBYE: {} break;

        case MSB_HELLO: {
            auto *p = &_header->hello;
            Read_To_Ptr(User_ID, &p->user, node);
            Read_To_Ptr(MS_Client_Type, &p->client_type, node);
        } break;

        case MSB_PLACE_ORDER: {
            auto *p = &_header->place_order;
            Read_To_Ptr(Money,        &p->price, node);
            Read_To_Ptr(bool,         &p->is_buy_order, node);

            if(p->is_buy_order) {
                auto *x = &p->buy;
                Read_To_Ptr(Item_Type_ID, &x->item_type, node);
            } else {
                auto *x = &p->sell;
                Read_To_Ptr(Item_ID, &x->item_id, node);
            }
        } break;

		case MSB_SET_WATCHED_ARTICLE: {
			auto *p = &_header->set_watched_article;

			Read_To_Ptr(Item_Type_ID, &p->article, node);
		} break;

        default: Assert(false); break;
    }
    
    return true;
}



bool enqueue_MSB_HELLO_packet(Network_Node *node, User_ID user, MS_Client_Type client_type)
{
    begin_outbound_packet(node);
    {
        Write(MSB_Packet_Type, MSB_HELLO, node);
        //--

        Write(User_ID, user, node);
        Write(MS_Client_Type, client_type, node);
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_MSB_GOODBYE_packet(Network_Node *node)
{
    begin_outbound_packet(node);
    {
        Write(MSB_Packet_Type, MSB_GOODBYE, node);
        //--
    }
    end_outbound_packet(node);
    return true;
}

bool send_MSB_GOODBYE_packet_now(Network_Node *node)
{
    Send_Now_NoArgs(MSB_GOODBYE, node);
    return true;
}


bool enqueue_MSB_PLACE_ORDER_packet(Network_Node *node, Money price, bool is_buy_order, Item_ID item_id_to_sell, Item_Type_ID item_type_to_buy)
{
    begin_outbound_packet(node);
    {
        Write(MSB_Packet_Type, MSB_PLACE_ORDER, node);
        //--
        Write(Money,        price, node);
        Write(bool,         is_buy_order, node);

        if(is_buy_order) {
            Write(Item_Type_ID, item_type_to_buy, node);
        } else {
            Write(Item_ID, item_id_to_sell, node);
        }
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_MSB_SET_WATCHED_ARTICLE_packet(Network_Node *node, Item_Type_ID article)
{
    begin_outbound_packet(node);
    {
        Write(MSB_Packet_Type, MSB_SET_WATCHED_ARTICLE, node);
        //--
        Write(Item_Type_ID, article, node);
    }
    end_outbound_packet(node);
    return true;
}



#endif // MARKET_SERVER_BOUND_INCLUDED
