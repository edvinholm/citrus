
#ifndef ROOM_SERVER_BOUND_INCLUDED
#define ROOM_SERVER_BOUND_INCLUDED



#define Write_RSB_Header(Packet_Type, Node_Ptr)                  \
    Fail_If_True(!write_RSB_Packet_Header({Packet_Type}, Node_Ptr))


// @Norelease TODO: Check that it is a valid type.
bool read_RSB_Packet_Type(RSB_Packet_Type *_type, Network_Node *node)
{
    u64 i;
    if(!read_u64(&i, node)) return false;
    *_type = (RSB_Packet_Type)i;
    return true;
}


bool write_RSB_Packet_Type(RSB_Packet_Type type, Network_Node *node)
{
    return write_u64(type, node);
}


// @Norelease TODO: Check that it is a valid type.
bool read_RSB_Developer_Packet_Type(RSB_Developer_Packet_Type *_type, Network_Node *node)
{
    u32 i;
    if(!read_u32(&i, node)) return false;
    *_type = (RSB_Developer_Packet_Type)i;
    return true;
}

bool write_RSB_Developer_Packet_Type(RSB_Developer_Packet_Type type, Network_Node *node)
{
    return write_u32(type, node);
}



bool read_RSB_Packet_Header(RSB_Packet_Header *_header, Network_Node *node)
{
    Zero(*_header);
    Read_To_Ptr(RSB_Packet_Type, &_header->type, node);
    Read_To_Ptr(u32,             &_header->id,   node);

    switch(_header->type)
    {
        case RSB_HELLO: {
            auto *p = &_header->hello;
            Read_To_Ptr(Room_ID, &p->room, node);
            Read_To_Ptr(User_ID, &p->as_user, node);
        } break;

        case RSB_GOODBYE: {} break;

        case RSB_CLICK_TILE: {
            auto *p = &_header->click_tile;
            Read_To_Ptr(u64,     &p->tile_ix, node);
        } break;

        case RSB_PLAYER_ACTION: {
            auto *p = &_header->player_action;
            Read_To_Ptr(Player_Action, p, node);
        } break;

        case RSB_CHAT: {
            auto *p = &_header->chat;
            Read_To_Ptr(String, &p->message_text, node);
        } break;

        case RSB_PLAYER_ACTION_DEQUEUE: {
            auto *p = &_header->player_action_dequeue;
            Read_To_Ptr(Player_Action_ID, &p->action_id, node);
        } break;

        case RSB_PLAYER_ACTION_QUEUE_PAUSE: {
            auto *p = &_header->player_action_queue_pause;
            Read_To_Ptr(bool, &p->remove, node);
            Read_To_Ptr(Player_Action_ID, &p->action_after, node);
        } break;

        case RSB_DEVELOPER: {
            auto *p = &_header->developer;
            Read_To_Ptr(RSB_Developer_Packet_Type, &p->type, node);
            switch(p->type) {
                case RSB_DEV_ADD_ENTITY: {
                    auto *x = &p->add_entity;
                    Read_To_Ptr(Entity, &x->entity, node);
                } break;

                default: Assert(false); return false;
            }
        } break;

        default: Assert(false); return false;
    }
    
    return true;
}


bool enqueue_RSB_HELLO_packet(Network_Node *node, Room_ID room, User_ID as_user)
{
    begin_outbound_packet(node);
    {
        Write(RSB_Packet_Type, RSB_HELLO, node);
        Write(u32, 0, node); // Packet ID
        //--

        Write(Room_ID, room, node);
        Write(User_ID, as_user, node);
    }
    end_outbound_packet(node);
    return true;
}

bool enqueue_RSB_GOODBYE_packet(Network_Node *node)
{
    begin_outbound_packet(node);
    {
        Write(RSB_Packet_Type, RSB_GOODBYE, node);
        Write(u32, 0, node); // Packet ID
        //--
    }
    end_outbound_packet(node);
    return true;
}

bool send_RSB_GOODBYE_packet_now(Network_Node *node)
{
    Send_Now_NoArgs(RSB_GOODBYE, node);
    return true;
}


bool enqueue_RSB_CLICK_TILE_packet(Network_Node *node, u64 tile_ix)
{
    begin_outbound_packet(node);
    defer(end_outbound_packet(node););
    
    Write(RSB_Packet_Type, RSB_CLICK_TILE, node);
    Write(u32, 0, node); // Packet ID
    //--

    Write(u64,  tile_ix, node);
        
    return true;
}

bool enqueue_RSB_PLAYER_ACTION_packet(Network_Node *node, u32 packet_id, Player_Action action)
{
    begin_outbound_packet(node);
    defer(end_outbound_packet(node););
    
    Write(RSB_Packet_Type, RSB_PLAYER_ACTION, node);
    Write(u32, packet_id, node);
    //--

    Write(Player_Action, action, node);
    
    return true;
}


bool enqueue_RSB_CHAT_packet(Network_Node *node, String message_text)
{    
    begin_outbound_packet(node);
    defer(end_outbound_packet(node););
    
    Write(RSB_Packet_Type, RSB_CHAT, node);
    Write(u32, 0, node); // Packet ID
    //--

    Write(String, message_text, node);
        
    return true;
}

bool enqueue_RSB_PLAYER_ACTION_DEQUEUE_packet(Network_Node *node, Player_Action_ID action_id)
{
    begin_outbound_packet(node);
    defer(end_outbound_packet(node););
    
    Write(RSB_Packet_Type, RSB_PLAYER_ACTION_DEQUEUE, node);
    Write(u32, 0, node); // Packet ID
    //--

    Write(Player_Action_ID, action_id, node);
        
    return true;
}


bool enqueue_RSB_PLAYER_ACTION_QUEUE_PAUSE_packet(Network_Node *node, bool remove, Player_Action_ID id_of_action_after)
{
    begin_outbound_packet(node);
    defer(end_outbound_packet(node););
    
    Write(RSB_Packet_Type, RSB_PLAYER_ACTION_QUEUE_PAUSE, node);
    Write(u32, 0, node); // Packet ID
    //--

    Write(bool, remove, node);
    Write(Player_Action_ID, id_of_action_after, node);
        
    return true;
}

bool enqueue_RSB_DEVELOPER_packet(Network_Node *node, RSB_Developer_Packet *packet)
{
    begin_outbound_packet(node);
    defer(end_outbound_packet(node););

    Write(RSB_Packet_Type, RSB_DEVELOPER, node);
    Write(u32, 0, node); // Packet ID
    //--

    Write(RSB_Developer_Packet_Type, packet->type, node);
    switch(packet->type) {
        case RSB_DEV_ADD_ENTITY: {
            Write(Entity, &packet->add_entity.entity, node);
        } break;

        default: Assert(false); return false;
    }

    return true;
}


#endif // ROOM_SERVER_BOUND_INCLUDED
