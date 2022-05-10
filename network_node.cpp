
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



bool read_s8(s8 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}

bool read_s16(s16 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}

bool read_s32(s32 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}

bool read_s64(s64 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}



bool read_u8(u8 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}


bool read_u16(u16 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}


bool read_u32(u32 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}


bool read_u64(u64 *_i, Network_Node *node)
{
    return read_bytes(_i, sizeof(*_i), node);
}


bool read_float(float *_f, Network_Node *node)
{
    return read_bytes(_f, sizeof(*_f), node);
}


bool read_bool(bool *_b, Network_Node *node)
{
    Read(u8, i, node);
    *_b = (i == 0) ? false : true;
    return true;
}



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

bool read_String(String *_str, Network_Node *node)
{
    u64 length;
    if(!read_u64(&length, node)) return false;
    _str->length = length;
    return place_data_pointer(&_str->data, _str->length, node);
}





bool write_bytes(void *data, u64 length, Network_Node *node)
{
    auto *buf = &node->send_buffer;
    if(!ensure_size(buf->packet_size + length, buf)) return false;
    memcpy(buf->data + buf->packet_size, data, length);
    buf->packet_size += length;

    return true;
}



bool write_s8(s8 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}


bool write_s16(s16 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}


bool write_s32(s32 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}


bool write_s64(s64 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}




bool write_u8(u8 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}


bool write_u16(u16 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}


bool write_u32(u32 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}


bool write_u64(u64 i, Network_Node *node)
{
    return write_bytes(&i, sizeof(i), node);
}


bool write_float(float f, Network_Node *node)
{
    Assert(sizeof(f) == sizeof(u32));
    Assert(sizeof(f) == 4);
    u32 i;
    memcpy(&i, &f, 4);
    return write_u32(i, node);
}



bool write_bool(bool b, Network_Node *node)
{
    return write_u8(b ? 1 : 0, node);
}



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


bool write_String(String str, Network_Node *node)
{
    // @Norelease: TODO @Security: Assert str.length < max length for strings... See note in read_String.
    if(!write_u64(str.length, node)) return false;
    if(!write_bytes(str.data, str.length, node)) return false;

    return true;
}






enum Node_Type {
    NODE_CLIENT,
    NODE_MASTER,
    NODE_ROOM,
    NODE_USER,
    NODE_MARKET
};

struct Node_Connection_Arguments
{
    union {
        struct {} client;
        struct {} master;
        struct {
            Room_ID room;
            User_ID as_user;
        } room;
        struct {
            User_ID user;
            US_Client_Type client_type;
            u32 client_node_id; // The client's node ID if client_type != PLAYER
        } user;
        struct {
            User_ID user;
            MS_Client_Type client_type;
        } market;
    };
};


bool connect_to_node(Network_Node *node, Node_Type type, Node_Connection_Arguments arguments)
{
    Assert(type != NODE_CLIENT);
    
    Assert(!node->connected);
    
    node->last_connect_attempt_failed = true; // We set this to false before returning true.

    u16 port;
    
    switch(type) { // @Jai: #complete
        case NODE_CLIENT: Assert(false); return false;
        case NODE_MASTER: Assert(!"Not yet implemented"); return false;
        case NODE_ROOM: {
            port = ROOM_SERVER_PORT;
        } break;
        case NODE_USER: {
            port = USER_SERVER_PORT;
        } break;
        case NODE_MARKET: {
            port = MARKET_SERVER_PORT;
        } break;
        default: Assert(false); return false;
    }
    
    Socket socket;
    
    if(!platform_create_tcp_socket(&socket)) {
        Debug_Print("Unable to create socket.\n");
        return false;
    }

    if(!platform_connect_socket(&socket, "127.0.0.1", port)) {
        Debug_Print("Unable to connect socket to server. WSA Error: %d\n", WSAGetLastError());
        return false;
    }

    reset_network_node(node, socket);                 
    platform_set_socket_read_timeout(&node->socket, 5 * 1000);

    switch(type) { // @Jai: #complete
        case NODE_CLIENT: Assert(false); return false;
        case NODE_MASTER: Assert(!"Not yet implemented"); return false;
            
        case NODE_ROOM: {
            auto &args = arguments.room;
            
            Send_Now(RSB_HELLO, node, args.room, args.as_user);
            
            RCB_Packet_Header header;
    
            Fail_If_True(!expect_type_of_next_rcb_packet(RCB_HELLO, node, &header));
            Fail_If_True(header.hello.connect_status != ROOM_CONNECT__REQUEST_RECEIVED);
    
            Fail_If_True(!expect_type_of_next_rcb_packet(RCB_HELLO, node, &header));
            Fail_If_True(header.hello.connect_status != ROOM_CONNECT__CONNECTED);
        } break;
            
        case NODE_USER: {
            auto &args = arguments.user;
            
            Send_Now(USB_HELLO, node, args.user, args.client_type, args.client_node_id);
            UCB_Packet_Header header;
            
            Fail_If_True(!expect_type_of_next_ucb_packet(UCB_HELLO, node, &header));
            Fail_If_True(header.hello.connect_status != USER_CONNECT__CONNECTED);
        } break;
            
        case NODE_MARKET: {
            auto &args = arguments.market;
            
            Send_Now(MSB_HELLO, node, args.user, args.client_type);
            MCB_Packet_Header header;
            
            Fail_If_True(!expect_type_of_next_mcb_packet(MCB_HELLO, node, &header));
            Fail_If_True(header.hello.connect_status != MARKET_CONNECT__CONNECTED);
        } break;
            
        default: Assert(false); return false;
    }

    platform_set_socket_read_timeout(&node->socket, 1000);

    node->connected = true;
    node->last_connect_attempt_failed = false;
    return true;
}
