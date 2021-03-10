
#ifndef NETWORK_NODE_INCLUDED
#define NETWORK_NODE_INCLUDED

#define Enqueue(Packet_Type, Node_Ptr, ...) \
    Fail_If_True(!enqueue_##Packet_Type##_packet(Node_Ptr, __VA_ARGS__));   \
    
#define Enqueue_NoArgs(Packet_Type, Node_Ptr)                        \
    Fail_If_True(!enqueue_##Packet_Type##_packet(Node_Ptr));    \
    
#define Send_Now(Packet_Type, Node_Ptr, ...)                            \
    Enqueue(Packet_Type, Node_Ptr, __VA_ARGS__);                        \
    Assert((Node_Ptr)->packet_queue.n == 1);                            \
    Fail_If_True(!send_outbound_packets(Node_Ptr))

// @Jai @Cleanup this stupid poop.
#define Send_Now_NoArgs(Packet_Type, Node_Ptr)                  \
    Enqueue_NoArgs(Packet_Type, Node_Ptr);                        \
    Assert((Node_Ptr)->packet_queue.n == 1);                      \
    Fail_If_True(!send_outbound_packets(Node_Ptr))


#define Data_Ptr(Ptr, Length, Node_Ptr)                                 \
    Fail_If_True(!place_data_pointer((u8 **)&Ptr, Length, Node_Ptr))


struct Network_Node_Buffer: public Memory_Buffer
{
    s64 caret; // Only used when reading.
    s64 packet_size; // When sending, this is the size of all packets together.
};

struct Network_Node
{
    // TODO @Norelease: Don't have the buffers in the node struct.
    //                  Have pointers to them here instead, or something...
    //                  and share them between multiple nodes where possible.
    Network_Node_Buffer receive_buffer; // NOTE: This holds at most one packet.
    Network_Node_Buffer send_buffer;    // NOTE: This holds at most one packet.

    struct Packet {
        s64 start;
        s64 size;
    };
    
    s64 current_outbound_packet_start_plus_one;
    Array<Packet, ALLOC_MALLOC> packet_queue;
    
    Socket socket;
};


void reset_network_node_buffer(Network_Node_Buffer *buf)
{
    buf->packet_size = 0;
    buf->caret = 0;
}

void reset_network_node(Network_Node *node, Socket socket)
{
    reset_network_node_buffer(&node->receive_buffer);
    reset_network_node_buffer(&node->send_buffer);

    node->current_outbound_packet_start_plus_one = 0;
    node->packet_queue.n = 0;

    node->socket = socket;
}


bool receive_next_network_node_packet(Network_Node *node, bool *_error, bool block = false)
{   
    if(!block) {
        if(!platform_socket_has_bytes_to_read(&node->socket, _error)) {
            return false;
        }
    }

    *_error = true;

    auto *buf = &node->receive_buffer;
    buf->caret = 0;
    
    Read(u64, packet_size, &node->socket);
    
    ensure_size(packet_size, buf);
    Read_Bytes(buf->data, packet_size, &node->socket);

    buf->packet_size = packet_size;

    *_error = false;
    return true;
}



bool write_network_packet(u8 *payload, u64 size, Socket *sock)
{
    Write(u64, size, sock);
    Write_Bytes(payload, size, sock);
    return true;
}

void begin_outbound_packet(Network_Node *node)
{
    Assert(node->current_outbound_packet_start_plus_one == 0);
    node->current_outbound_packet_start_plus_one = node->send_buffer.packet_size + 1;
}

bool end_outbound_packet(Network_Node *node)
{
    Assert(node->current_outbound_packet_start_plus_one > 0);

    auto start = node->current_outbound_packet_start_plus_one-1;
    auto size  = node->send_buffer.packet_size - start;

    Assert(size > 0);

    Network_Node::Packet packet = { start, size };
    array_add(node->packet_queue, packet);

    node->current_outbound_packet_start_plus_one = 0;
    
    return true;
}

bool send_outbound_packets(Network_Node *node)
{
    bool success = true;
    
    Assert(node->current_outbound_packet_start_plus_one == 0);
    
    auto *buf = &node->send_buffer;   
    
    for(int i = 0; i < node->packet_queue.n; i++)
    {
        auto &packet = node->packet_queue[i];
        
        Assert(packet.start >= 0);
        Assert(packet.start < buf->packet_size);

        Assert(packet.size > 0);
        Assert(packet.start + packet.size <= buf->packet_size);
                
        if(!write_network_packet(buf->data + packet.start, packet.size, &node->socket)) {
            success = false;
            break;
        }
    }

    node->packet_queue.n = 0;
    reset_network_node_buffer(buf);
    
    return success;
}








bool can_read_bytes(s64 length, Network_Node *node)
{
    auto *buffer = &node->receive_buffer;
    auto new_caret = buffer->caret + length;
    return (new_caret <= buffer->packet_size);
}
                    
bool read_bytes(void *_data, s64 length, Network_Node *node)
{
    Fail_If_True(!can_read_bytes(length, node));
    
    auto *buffer = &node->receive_buffer;
    memcpy(_data, buffer->data + buffer->caret, length);
    buffer->caret += length;
    
    return true;
}

bool place_data_pointer(u8 **_pointer, s64 data_length, Network_Node *node)
{
    Fail_If_True(!can_read_bytes(data_length, node));
    
    auto *buffer = &node->receive_buffer;
    *_pointer = buffer->data + buffer->caret;
    buffer->caret += data_length;
    
    return true;
}



inline
bool read_s8(s8 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}

inline
bool read_s16(s16 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}

inline
bool read_s32(s32 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}

inline
bool read_s64(s64 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}


inline
bool read_u8(u8 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}

inline
bool read_u16(u16 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}

inline
bool read_u32(u32 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}

inline
bool read_u64(u64 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}

inline
bool read_float(float *_f, Network_Node *node)
{
    return read_bytes(_f, sizeof(*_f), node);
}

inline
bool read_bool(bool *_b, Network_Node *node)
{
    Read(u8, i, node);
    *_b = (i == 0) ? false : true;
    return true;
}


inline
bool read_Quat(Quat *_q, Network_Node *node)
{   
    Assert(sizeof(_q->x) == sizeof(u32));
    Assert(sizeof(_q->x) == 4);
    u32 x, y, z, w; 
    if(!read_u32(&x, node)) return false;
    if(!read_u32(&y, node)) return false;
    if(!read_u32(&z, node)) return false;
    if(!read_u32(&w, node)) return false;

    memcpy(&_q->x, &x, 4);
    memcpy(&_q->y, &y, 4);
    memcpy(&_q->z, &z, 4);
    memcpy(&_q->w, &w, 4);

    return true;
}


inline
bool read_v3(v3 *_u, Network_Node *node)
{
    Assert(sizeof(_u->x) == sizeof(u32));
    Assert(sizeof(_u->x) == 4);
    u32 x, y, z;    
    if(!read_u32(&x, node)) return false;
    if(!read_u32(&y, node)) return false;
    if(!read_u32(&z, node)) return false;

    memcpy(&_u->x, &x, 4);
    memcpy(&_u->y, &y, 4);
    memcpy(&_u->z, &z, 4);

    return true;
}

inline
bool read_v4(v4 *_u, Network_Node *node)
{
    Assert(sizeof(_u->x) == sizeof(u32));
    Assert(sizeof(_u->x) == 4);
    u32 x, y, z, w;    
    if(!read_u32(&x, node)) return false;
    if(!read_u32(&y, node)) return false;
    if(!read_u32(&z, node)) return false;
    if(!read_u32(&w, node)) return false;

    memcpy(&_u->x, &x, 4);
    memcpy(&_u->y, &y, 4);
    memcpy(&_u->z, &z, 4);
    memcpy(&_u->w, &w, 4);

    return true;
}

// NOTE: _str->data will point to a place in node->receive_buffer.data on success.
inline
bool read_String(String *_str, Network_Node *node)
{
    u64 length;
    if(!read_u64(&length, node)) return false;
    _str->length = length;
    return place_data_pointer(&_str->data, _str->length, node);
}




inline
bool write_bytes(void *data, u64 length, Network_Node *node)
{
    auto *buf = &node->send_buffer;
    if(!ensure_size(buf->packet_size + length, buf)) return false;
    memcpy(buf->data + buf->packet_size, data, length);
    buf->packet_size += length;

    return true;
}


inline
bool write_s8(s8 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}

inline
bool write_s16(s16 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}

inline
bool write_s32(s32 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}

inline
bool write_s64(s64 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}



inline
bool write_u8(u8 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}

inline
bool write_u16(u16 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}

inline
bool write_u32(u32 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}

inline
bool write_u64(u64 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}

inline
bool write_float(float f, Network_Node *node)
{
    Assert(sizeof(f) == sizeof(u32));
    Assert(sizeof(f) == 4);
    u32 i;
    memcpy(&i, &f, 4);
    return write_u32(i, node);
}


inline
bool write_bool(bool b, Network_Node *node)
{
    return write_u8(b ? 1 : 0, node);
}


inline
bool write_Quat(Quat q, Network_Node *node)
{
    Assert(sizeof(q.x) == sizeof(u32));
    Assert(sizeof(q.x) == 4);
    u32 x, y, z, w;
    memcpy(&x, &q.x, 4);
    memcpy(&y, &q.y, 4);
    memcpy(&z, &q.z, 4);
    memcpy(&w, &q.w, 4);

    if(!write_u32(x, node)) return false;
    if(!write_u32(y, node)) return false;
    if(!write_u32(z, node)) return false;
    if(!write_u32(w, node)) return false;

    return true;
}


inline
bool write_v3(v3 u, Network_Node *node)
{
    Assert(sizeof(u.x) == sizeof(u32));
    Assert(sizeof(u.x) == 4);
    u32 x, y, z;
    memcpy(&x, &u.x, 4);
    memcpy(&y, &u.y, 4);
    memcpy(&z, &u.z, 4);

    if(!write_u32(x, node)) return false;
    if(!write_u32(y, node)) return false;
    if(!write_u32(z, node)) return false;

    return true;
}


inline
bool write_v4(v4 u, Network_Node *node)
{
    Assert(sizeof(u.x) == sizeof(u32));
    Assert(sizeof(u.x) == 4);
    u32 x, y, z, w;
    memcpy(&x, &u.x, 4);
    memcpy(&y, &u.y, 4);
    memcpy(&z, &u.z, 4);
    memcpy(&w, &u.w, 4);

    if(!write_u32(x, node)) return false;
    if(!write_u32(y, node)) return false;
    if(!write_u32(z, node)) return false;
    if(!write_u32(w, node)) return false;

    return true;
}

inline
bool write_String(String str, Network_Node *node)
{
    // @Norelease: TODO @Security: Assert str.length < max length for strings... See note in read_String.
    if(!write_u64(str.length, node)) return false;
    if(!write_bytes(str.data, str.length, node)) return false;

    return true;
}







// User //
static_assert(sizeof(User_ID) == sizeof(u64));
bool read_User_ID(User_ID *_user_id, Network_Node *node)
{
    Read(u64, id, node);
    *_user_id = (User_ID)id;
    return true;
}

bool write_User_ID(User_ID user_id, Network_Node *node)
{
    Write(u64, user_id, node);
    return true;
}

static_assert(sizeof(Money) == sizeof(u64));
bool read_Money(Money *_money, Network_Node *node)
{
    Read(s64, i, node);
    *_money = (Money)i;
    return true;
}

bool write_Money(Money money, Network_Node *node)
{
    Write(s64, money, node);
    return true;
}




// World_Time //

static_assert(sizeof(World_Time) == sizeof(double));
                  
bool read_World_Time(World_Time *_time, Network_Node *node)
{
    Read(u64, time, node);
    *_time = (World_Time)time / 1000000.0;
    return true;
}

bool write_World_Time(World_Time time, Network_Node *node)
{
    Write(u64, time * 1000000.0, node);
    return true;
}



// Item //
// @Norelease TODO: Check that it is a valid type.
bool read_Item_Type_ID(Item_Type_ID *_type_id, Network_Node *node)
{
    Read(u32, type_id, node);
    *_type_id = (Item_Type_ID)type_id;
    return true;
}

bool write_Item_Type_ID(Item_Type_ID type_id, Network_Node *node)
{
    Write(u32, type_id, node);
    return true;
}

bool read_Item_ID(Item_ID *_id, Network_Node *node)
{
    Read(u64, origin, node);
    Read(u64, number, node);
    _id->origin = origin;
    _id->number = number;
    return true;
}

bool write_Item_ID(Item_ID id, Network_Node *node)
{
    Write(u64, id.origin, node);
    Write(u64, id.number, node);
    return true;
}

// @Norelease @Security: Check that it is a valid type.
bool read_Liquid_Type(Liquid_Type *_type, Network_Node *node)
{
    static_assert(sizeof(*_type) == sizeof(u8));
    Read(u8, type, node);
    *_type = (Liquid_Type)type;
    return true;
}

bool write_Liquid_Type(Liquid_Type type, Network_Node *node)
{
    static_assert(sizeof(type) == sizeof(u8));
    Write(u8, type, node);
    return true;
}


bool read_Liquid_Fraction(Liquid_Fraction *_f, Network_Node *node)
{
    static_assert(sizeof(*_f) == sizeof(u32));
    Read_To_Ptr(u32, _f, node);
    return true;
}

bool write_Liquid_Fraction(Liquid_Fraction f, Network_Node *node)
{
    static_assert(sizeof(f) == sizeof(u32));
    Write(u32, f, node);
    return true;
}


bool read_Liquid(Liquid *_lq, Network_Node *node)
{
    Read_To_Ptr(Liquid_Type, &_lq->type, node);

    if(_lq->type == LQ_YEAST_WATER) {
        auto &yw = _lq->yeast_water;
        Read_To_Ptr(Liquid_Fraction, &yw.yeast, node);
        Read_To_Ptr(Liquid_Fraction, &yw.nutrition, node);
    }
    
    return true;
}

bool write_Liquid(Liquid lq, Network_Node *node)
{
    Write(Liquid_Type, lq.type, node);

    if(lq.type == LQ_YEAST_WATER) {
        auto &yw = lq.yeast_water;
        Write(Liquid_Fraction, yw.yeast, node);
        Write(Liquid_Fraction, yw.nutrition, node);
    }
    
    return true;
}


bool read_Liquid_Amount(Liquid_Amount *_amt, Network_Node *node)
{
    static_assert(sizeof(*_amt) == sizeof(u32));
    Read_To_Ptr(u32, _amt, node);
    return true;
}

bool write_Liquid_Amount(Liquid_Amount amt, Network_Node *node)
{
    static_assert(sizeof(amt) == sizeof(u32));
    Write(u32, amt, node);
    return true;
}


bool read_Liquid_Container(Liquid_Container *_lc, Network_Node *node)
{
    Read_To_Ptr(Liquid,        &_lc->liquid, node);
    Read_To_Ptr(Liquid_Amount, &_lc->amount, node);
    return true;
}

bool write_Liquid_Container(Liquid_Container lc, Network_Node *node)
{
    Write(Liquid,        lc.liquid, node);
    Write(Liquid_Amount, lc.amount, node);
    return true;
}

bool read_Item(Item *_item, Network_Node *node)
{
    Read_To_Ptr(Item_ID,      &_item->id,    node);
    Read_To_Ptr(Item_Type_ID, &_item->type,  node);
    Read_To_Ptr(User_ID,      &_item->owner, node);

    switch(_item->type) {
        case ITEM_PLANT: {
            auto *x = &_item->plant;
            Read_To_Ptr(float, &x->grow_progress, node);
        } break;
    }

    // @Speed? (Accessing item_types[..] which might not be in cache? Or is it?
    if(item_types[_item->type].flags & ITEM_IS_LQ_CONTAINER) {
        Read_To_Ptr(Liquid_Container, &_item->liquid_container, node);
    }
    
    return true;
}

bool write_Item(Item item, Network_Node *node)
{
    Write(Item_ID,      item.id,    node);
    Write(Item_Type_ID, item.type,  node);
    Write(User_ID,      item.owner, node);

    switch(item.type) {
        case ITEM_PLANT: {
            auto *x = &item.plant;
            Write(float, x->grow_progress, node);
        } break;
    }

    // @Speed? (Accessing item_types[..] which might not be in cache? Or is it?
    if(item_types[item.type].flags & ITEM_IS_LQ_CONTAINER) {
        Write(Liquid_Container, item.liquid_container, node);
    }
    
    return true;
}

bool read_Inventory_Slot_Flags(Inventory_Slot_Flags *_flags, Network_Node *node)
{
    Read(u8, flags, node);
    *_flags = (Inventory_Slot_Flags)flags;
    return true;
}
bool write_Inventory_Slot_Flags(Inventory_Slot_Flags flags, Network_Node *node)
{
    Write(u8, flags, node);
    return true;
}

bool read_Inventory_Slot(Inventory_Slot *_slot, Network_Node *node)
{
    Read_To_Ptr(Inventory_Slot_Flags, &_slot->flags, node);
    Read_To_Ptr(Item, &_slot->item, node);
    return true;
}

bool write_Inventory_Slot(Inventory_Slot *slot, Network_Node *node)
{
    Write(Inventory_Slot_Flags, slot->flags, node);
    Write(Item, slot->item, node);
    return true;
}



// Market //

// @Norelease TODO: Check that it is a valid value.
bool read_Price_Period(Price_Period *_period, Network_Node *node)
{
    Read(u8, period, node);
    *_period = (Price_Period)period;
    return true;
}

bool write_Price_Period(Price_Period period, Network_Node *node)
{
    Write(u8, period, node);
    return true;
}

// @Norelease TODO: Check that it is a valid value.
bool read_Market_View_Target_Type(Market_View_Target_Type *_type, Network_Node *node)
{
    Read(u8, type, node);
    *_type = (Market_View_Target_Type)type;
    return true;
}

bool write_Market_View_Target_Type(Market_View_Target_Type type, Network_Node *node)
{
    Write(u8, type, node);
    return true;
}

bool read_Market_View_Target(Market_View_Target *_target, Network_Node *node)
{
    Read_To_Ptr(Market_View_Target_Type, &_target->type, node);

    switch(_target->type)
    {
        case MARKET_VIEW_TARGET_ARTICLE: {
            auto *x = &_target->article;
            Read_To_Ptr(Item_Type_ID, &x->article,      node);
            Read_To_Ptr(Price_Period, &x->price_period, node);
        } break;

        case MARKET_VIEW_TARGET_ORDERS: {
            
        } break;
    }

    return true;
}

bool write_Market_View_Target(Market_View_Target target, Network_Node *node)
{
    Write(Market_View_Target_Type, target.type, node);

    switch(target.type)
    {
        case MARKET_VIEW_TARGET_ARTICLE: {
            auto *x = &target.article;
            Write(Item_Type_ID, x->article,      node);
            Write(Price_Period, x->price_period, node);
        } break;

        case MARKET_VIEW_TARGET_ORDERS: {
            
        } break;
    }

    return true;
}


bool read_Market_View(S__Market_View *_view, Network_Node *node)
{
    Read_To_Ptr(Market_View_Target, &_view->target, node);

    switch(_view->target.type)
    {
        case MARKET_VIEW_TARGET_ARTICLE: {
            auto *x = &_view->article;
            
            if(_view->target.article.article != ITEM_NONE_OR_NUM)
            {
                Read_To_Ptr(u16, &x->num_prices, node);
                Fail_If_True(x->num_prices > ARRLEN(x->prices));
                
                for(int i = 0; i < x->num_prices; i++) {
                    Read_To_Ptr(Money, &x->prices[i], node);
                }
            }
        } break;

        case MARKET_VIEW_TARGET_ORDERS: {
            auto *x = &_view->orders;
        } break;
    }
    
    return true;
}

bool write_Market_View(S__Market_View *view, Network_Node *node)
{
    Write(Market_View_Target, view->target, node);

    switch(view->target.type)
    {
        case MARKET_VIEW_TARGET_ARTICLE: {
            auto *x = &view->article;
            
            if(view->target.article.article != ITEM_NONE_OR_NUM)
            {
                Fail_If_True(x->num_prices > ARRLEN(x->prices));
                Write(u16, x->num_prices, node);
                
                for(int i = 0; i < x->num_prices; i++) {
                    Write(Money, x->prices[i], node);
                }
            }
        } break;

        case MARKET_VIEW_TARGET_ORDERS: {
            auto *x = &view->orders;
        } break;
    }

    return true;
}



// Room //
bool read_Room_ID(Room_ID *_room_id, Network_Node *node)
{
    Read(u64, id, node);
    *_room_id = (Room_ID)id;
    return true;
}

bool write_Room_ID(Room_ID room_id, Network_Node *node)
{
    Write(u64, room_id, node);
    return true;
}


// Chat //
bool read_Chat_Message(Chat_Message *_message, Network_Node *node)
{
    Read_To_Ptr(World_Time, &_message->t,    node);
    Read_To_Ptr(User_ID,    &_message->user, node);
    Read_To_Ptr(String,     &_message->text, node);
    return true;
}

bool write_Chat_Message(Chat_Message message, Network_Node *node)
{
    Write(World_Time, message.t,    node);
    Write(User_ID,    message.user, node);
    Write(String,     message.text, node);
    return true;
}

// Chess (@Cleanup: Move to some chess file?) //
// @Norelease TODO: Check that it is a valid type.
bool read_Chess_Action_Type(Chess_Action_Type *_type, Network_Node *node)
{
    Read(u8, type, node);
    *_type = (Chess_Action_Type)type;
    return true;
}

bool write_Chess_Action_Type(Chess_Action_Type type, Network_Node *node)
{
    Write(u8, type, node);
    return true;
}


// Entity //
// @Norelease TODO: Check that it is a valid type.
bool read_Entity_Type(Entity_Type *_type, Network_Node *node)
{
    Read(u8, type, node);
    *_type = (Entity_Type)type;
    return true;
}

bool write_Entity_Type(Entity_Type type, Network_Node *node)
{
    Write(u8, type, node);
    return true;
}

bool read_Entity_ID(Entity_ID *_id, Network_Node *node)
{
    Read(u64, id, node);
    *_id = (Entity_ID)id;
    return true;
}

bool write_Entity_ID(Entity_ID id, Network_Node *node)
{
    Write(u64, id, node);
    return true;
}


// @Norelease TODO: Check that it is a valid type.
bool read_Entity_Action_Type(Entity_Action_Type *_type, Network_Node *node)
{
    Read(u32, i, node);
    *_type = (Entity_Action_Type)i;
    return true;
}

bool write_Entity_Action_Type(Entity_Action_Type type, Network_Node *node)
{
    Write(u32, type, node);
    return true;
}


bool read_Entity_Action(Entity_Action *_action, Network_Node *node)
{
    Read_To_Ptr(Entity_Action_Type, &_action->type, node);

    switch(_action->type) {
        case ENTITY_ACT_SET_POWER_MODE: {
            auto *x = &_action->set_power_mode;
            Read_To_Ptr(bool, &x->set_to_on, node);
        } break;
            
        case ENTITY_ACT_SIT_OR_UNSIT: {
            auto *x = &_action->sit_or_unsit;
            Read_To_Ptr(bool, &x->unsit, node);
        } break;

        case ENTITY_ACT_CHESS: {
            auto *x = &_action->chess;

            Read_To_Ptr(Chess_Action_Type, &x->type, node);
            switch(x->type) {
                case CHESS_ACT_MOVE: {
                    // @Norelease: Check that square indices are in range!
                    Read_To_Ptr(u8, &x->move.from, node);
                    Read_To_Ptr(u8, &x->move.to, node);
                } break;

                case CHESS_ACT_JOIN: {
                    Read_To_Ptr(bool, &x->join.as_black, node);
                } break;
            }
        } break;
    }
    
    return true;
}

bool write_Entity_Action(Entity_Action action, Network_Node *node)
{
    Write(Entity_Action_Type, action.type, node);

    switch(action.type) {
        case ENTITY_ACT_SET_POWER_MODE: {
            auto *x = &action.set_power_mode;
            Write(bool, x->set_to_on, node);
        } break;
                        
        case ENTITY_ACT_SIT_OR_UNSIT: {
            auto *x = &action.sit_or_unsit;
            Write(bool, x->unsit, node);
        } break;

        case ENTITY_ACT_CHESS: {
            auto *x = &action.chess;

            Write(Chess_Action_Type, x->type, node);
            switch(x->type) {
                case CHESS_ACT_MOVE: {
                    Write(u8, x->move.from, node);
                    Write(u8, x->move.to, node);
                } break;

                case CHESS_ACT_JOIN: {
                    Write(bool, x->join.as_black, node);
                } break;
            }
            
        } break;
    }
    
    return true;
}


// @Norelease TODO: Check that it is a valid type.
bool read_Player_Action_Type(Player_Action_Type *_type, Network_Node *node)
{
    Read(u32, i, node);
    *_type = (Player_Action_Type)i;
    return true;
}

bool write_Player_Action_Type(Player_Action_Type type, Network_Node *node)
{
    Write(u32, type, node);
    return true;
}


bool read_Player_Action(Player_Action *_action, Network_Node *node)
{
    Read_To_Ptr(Player_Action_Type, &_action->type, node);

    switch(_action->type) { // @Jai: #complete
        case PLAYER_ACT_ENTITY: {
            auto *x = &_action->entity;
            Read_To_Ptr(Entity_ID,     &x->target, node);
            Read_To_Ptr(Entity_Action, &x->action, node);
        } break;

        case PLAYER_ACT_WALK: {
            auto *x = &_action->walk;
            Read_To_Ptr(v3,         &x->p1, node);
        } break;
            
        case PLAYER_ACT_PUT_DOWN: {
            auto *x = &_action->put_down;
            Read_To_Ptr(v3,   &x->tp, node);
            Read_To_Ptr(Quat, &x->q,  node);
        } break;
            
        case PLAYER_ACT_PLACE_FROM_INVENTORY: {
            auto *x = &_action->place_from_inventory;
            Read_To_Ptr(Item_ID, &x->item, node);
            Read_To_Ptr(v3,      &x->tp, node);
            Read_To_Ptr(Quat,    &x->q, node);
        } break;

        default: Assert(false); return false;
    }

    return true;
}

bool write_Player_Action(Player_Action action, Network_Node *node)
{
    Write(Player_Action_Type, action.type, node);

    switch(action.type) { // @Jai: #complete
        case PLAYER_ACT_ENTITY: {
            auto *x = &action.entity;
            Write(Entity_ID,     x->target, node);
            Write(Entity_Action, x->action, node);
        } break;

        case PLAYER_ACT_WALK: {
            auto *x = &action.walk;
            Write(v3, x->p1, node);
        } break;
            
        case PLAYER_ACT_PUT_DOWN: {
            auto *x = &action.put_down;
            Write(v3,   x->tp, node);
            Write(Quat, x->q,  node);
        } break;

        case PLAYER_ACT_PLACE_FROM_INVENTORY: {
            auto *x = &action.place_from_inventory;
            Write(Item_ID, x->item, node); // @Norelease @Robustness: Check that this is not NO_ITEM
            Write(v3,      x->tp, node);
            Write(Quat,    x->q,  node);
        } break;

        default: Assert(false); return false;
    }

    return true;
}

// @Norelease: Check that it is a valid value.
bool read_Chess_Player_Flags(Chess_Player_Flags *_flags, Network_Node *node)
{
    Read(u8, flags, node);
    *_flags = (Chess_Player_Flags)flags;
    return true;
}

bool write_Chess_Player_Flags(Chess_Player_Flags flags, Network_Node *node)
{
    Write(u8, flags, node);
    return true;
}

bool read_Chess_Player(Chess_Player *_player, Network_Node *node)
{
    Read_To_Ptr(User_ID,            &_player->user, node);
    Read_To_Ptr(Chess_Player_Flags, &_player->flags, node);
    return true;
};

bool write_Chess_Player(Chess_Player player, Network_Node *node)
{
    Write(User_ID,            player.user, node);
    Write(Chess_Player_Flags, player.flags, node);
    return true;
};


bool read_Chess_Move(Chess_Move *_move, Network_Node *node)
{
    Read_To_Ptr(u8, &_move->from, node);
    Read_To_Ptr(u8, &_move->to,   node);
    return true;
}

bool write_Chess_Move(Chess_Move move, Network_Node *node)
{
    Write(u8, move.from, node);
    Write(u8, move.to,   node);
    return true;
}

// @Norelease: Check that it is a valid value.
bool read_Chess_Special_Move(Chess_Special_Move *_special, Network_Node *node)
{
    Read(u8, special, node);
    *_special = (Chess_Special_Move)special;
    return true;
}

bool write_Chess_Special_Move(Chess_Special_Move special, Network_Node *node)
{
    Write(u8, special, node);
    return true;
}

// @Norelease: Check that all squares have valid values.
bool read_Chess_Board(Chess_Board *_board, Network_Node *node)
{
    static_assert(sizeof(_board->squares[0]) == 1);
    static_assert(sizeof(_board->squares)    == 8*8);
    
    Read_To_Ptr(Chess_Player, &_board->white_player, node);
    Read_To_Ptr(Chess_Player, &_board->black_player, node);
    
    Read_To_Ptr(bool, &_board->black_players_turn, node);

    Read_To_Ptr(u32, &_board->num_moves, node);
    Read_To_Ptr(Chess_Move,         &_board->last_move, node);
    Read_To_Ptr(Chess_Special_Move, &_board->last_move_special, node);
    
    Read_Bytes(_board->squares, 8*8, node);
    
    return true;
}

bool write_Chess_Board(Chess_Board *board, Network_Node *node)
{
    static_assert(sizeof(board->squares[0]) == 1);
    static_assert(sizeof(board->squares)    == 8*8);
    
    Write(Chess_Player, board->white_player, node);
    Write(Chess_Player, board->black_player, node);
    
    Write(bool, board->black_players_turn, node);

    Write(u32, board->num_moves, node);
    Write(Chess_Move,         board->last_move, node);
    Write(Chess_Special_Move, board->last_move_special, node);
    
    Write_Bytes(board->squares, 8*8, node);
    
    return true;
}


bool read_Entity(S__Entity *_entity, Network_Node *node)
{
    Zero(*_entity);
    
    Read_To_Ptr(Entity_ID,   &_entity->id,   node);
    Read_To_Ptr(Entity_Type, &_entity->type, node);
    
    Read_To_Ptr(Entity_ID, &_entity->held_by, node);
    Read_To_Ptr(Entity_ID, &_entity->holding, node);

    switch(_entity->type) {
        case ENTITY_ITEM: {
            auto *x = &_entity->item_e;
            
            Read_To_Ptr(v3,   &x->p, node);
            Read_To_Ptr(Quat, &x->q, node);
            
            Read_To_Ptr(Item, &x->item, node);

            Read_To_Ptr(Entity_ID, &x->locked_by, node);
            
            switch(x->item.type) {
                case ITEM_PLANT: {
                    auto *plant = &x->plant;
                    Read_To_Ptr(World_Time, &plant->t_on_plant,             node);
                    Read_To_Ptr(float,      &plant->grow_progress_on_plant, node);
                } break;

                case ITEM_MACHINE: {
                    auto *machine = &x->machine;
                    Read_To_Ptr(World_Time, &machine->start_t, node);
                    Read_To_Ptr(World_Time, &machine->stop_t,  node);
                } break;

                case ITEM_CHESS_BOARD: {
                    auto *board = &x->chess_board;
                    Read_To_Ptr(Chess_Board, board, node);
                } break;

                case ITEM_BLENDER: {
                    auto *blender = &x->blender;
                    Read_To_Ptr(World_Time, &blender->t_on_recipe_begin, node);
                    Read_To_Ptr(World_Time, &blender->recipe_duration, node);

                    // @Cleanup: Have a way of reading Static_Arrays... Maybe wait for @Jai.
                    Read_To_Ptr(s64, &blender->recipe_inputs.n, node);
                    Fail_If_True(blender->recipe_inputs.n > ARRLEN(blender->recipe_inputs.e));
                    for(s64 i = 0; i < blender->recipe_inputs.n; i++) {
                        Read_To_Ptr(Entity_ID, &blender->recipe_inputs.e[i], node);
                    }
                } break;
            }

            auto *type = &item_types[x->item.type];
            if(type->flags & ITEM_IS_LQ_CONTAINER) {
                Read_To_Ptr(World_Time, &x->lc_t0, node);
                Read_To_Ptr(World_Time, &x->lc_t1, node);
                Read_To_Ptr(Liquid_Container, &x->lc0, node);
                Read_To_Ptr(Liquid_Container, &x->lc1, node);
            }
            
        } break;
            
        case ENTITY_PLAYER: {
            auto *x = &_entity->player_e;
            Read_To_Ptr(User_ID, &x->user_id, node);

            // WALK PATH //
            Read_To_Ptr(World_Time, &x->walk_t0, node);

            Read_To_Ptr(u16, &x->walk_path_length, node);
            Fail_If_True(x->walk_path_length < 2);

            x->walk_path = (v3 *)tmp_alloc(sizeof(*x->walk_path) * x->walk_path_length);
            
            for(int i = 0; i < x->walk_path_length; i++) {
                Read_To_Ptr(v3, &x->walk_path[i], node);
            }
            //--

            Read_To_Ptr(u8, &x->action_queue_length, node);
            Fail_If_True(x->action_queue_length > ARRLEN(x->action_queue));
            for(int i = 0; i < x->action_queue_length; i++) {
                Read_To_Ptr(Player_Action, &x->action_queue[i], node);
            }

            Read_To_Ptr(Entity_ID, &x->sitting_on, node);
        } break;

        default: Assert(false); return false;
    }
    
    return true;
}

bool write_Entity(S__Entity *entity, Network_Node *node)
{
    Write(Entity_ID,   entity->id,   node);
    Write(Entity_Type, entity->type, node);
    
    Write(Entity_ID, entity->held_by, node);
    Write(Entity_ID, entity->holding, node);

    switch(entity->type) {
        case ENTITY_ITEM: {
            auto *x = &entity->item_e;
            
            Write(v3,   x->p, node);
            Write(Quat, x->q, node);
            
            Write(Item, x->item, node);
            
            Write(Entity_ID, x->locked_by, node);
            
            switch(x->item.type) {
                case ITEM_PLANT: {
                    auto *plant = &x->plant;
                    Write(World_Time, plant->t_on_plant,             node);
                    Write(float,      plant->grow_progress_on_plant, node);
                } break;

                case ITEM_MACHINE: {
                    auto *machine = &x->machine;
                    Write(World_Time, machine->start_t, node);
                    Write(World_Time, machine->stop_t, node);
                } break;
                    
                case ITEM_CHESS_BOARD: {
                    auto *board = &x->chess_board;
                    Write(Chess_Board, board, node);
                } break;
                    
                case ITEM_BLENDER: {
                    auto *blender = &x->blender;
                    Write(World_Time, blender->t_on_recipe_begin, node);
                    Write(World_Time, blender->recipe_duration, node);

                    // @Cleanup: Have a way of reading Static_Arrays... Maybe wait for @Jai.
                    Write(s64, blender->recipe_inputs.n, node);
                    for(s64 i = 0; i < blender->recipe_inputs.n; i++) {
                        Write(Entity_ID, blender->recipe_inputs.e[i], node);
                    }

                } break;
            }

            
            auto *type = &item_types[x->item.type];
            if(type->flags & ITEM_IS_LQ_CONTAINER) {
                Write(World_Time, x->lc_t0, node);
                Write(World_Time, x->lc_t1, node);
                Write(Liquid_Container, x->lc0, node);
                Write(Liquid_Container, x->lc1, node);
            }
            
        } break;

        case ENTITY_PLAYER: {
            auto *x = &entity->player_e;
            Write(User_ID, x->user_id, node);

            // WALK PATH //
            Fail_If_True(x->walk_path_length < 2);

            Write(World_Time, x->walk_t0,          node);
            Write(u16,        x->walk_path_length, node);
            for(int i = 0; i < x->walk_path_length; i++) {
                Write(v3, x->walk_path[i], node);
            }
            //--
            
            Fail_If_True(x->action_queue_length > ARRLEN(x->action_queue));
            Write(u8, x->action_queue_length, node);
            for(int i = 0; i < x->action_queue_length; i++) {
                Write(Player_Action, x->action_queue[i], node);
            }
            
            Write(Entity_ID, x->sitting_on, node);
        } break;

        default: Assert(false); return false;
    }

    return true;
}



// Tile //
bool read_Tile(Tile *_tile, Network_Node *node)
{
    Read(u8, i, node);
    *_tile = (Tile)i;
    return true;
}

bool write_Tile(Tile tile, Network_Node *node)
{
    return write_u8(tile, node);
}






// Transaction //
// @Norelease TODO: Check that it is a valid type.
bool read_Transaction_Message(Transaction_Message *_message, Network_Node *node)
{
    Read(u8, msg, node);
    *_message = (Transaction_Message)msg;
    return true;
}


bool write_Transaction_Message(Transaction_Message message, Network_Node *node)
{
    Write(u8, message, node);
    return true;
}











#endif // NETWORK_NODE_INCLUDED
