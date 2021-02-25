#ifndef ROOM_CLIENT_BOUND_INCLUDED
#define ROOM_CLIENT_BOUND_INCLUDED

enum Room_Connect_Status: u64 // @Cleanup: This does not need to be 64 bits (Change this here and in network code)
{
    ROOM_CONNECT__REQUEST_RECEIVED = 1,
    ROOM_CONNECT__CONNECTED        = 2,
    
    ROOM_CONNECT__INVALID_ROOM_ID  = 3
};

enum RCB_Packet_Type
{
    RCB_HELLO     = 1,
    RCB_GOODBYE   = 2,
    
    RCB_ROOM_INIT   = 3,
    RCB_ROOM_UPDATE = 4,
};


struct RCB_Packet_Header
{
    RCB_Packet_Type type;

    union {
        struct {
            Room_Connect_Status connect_status;
        } hello;
        
        struct {} goodbye;
        
        struct {
            World_Time time;
            u32 num_entities;
            Tile *tiles;
            Walk_Map walk_map;
        } room_init;
        
        struct {
            u32 tile0;
            u32 tile1;
            u32 num_entities;
            u16 num_chat_messages;
            Tile *tiles;
            Walk_Map walk_map;
        } room_update;
    };
};

bool read_Room_Connect_Status(Room_Connect_Status *_status, Network_Node *node)
{
    Read(u8, i, node);
    *_status = (Room_Connect_Status)i;
    return true;
}

bool write_Room_Connect_Status(Room_Connect_Status status, Network_Node *node)
{
    Write(u8, status, node);
    return true;
}



// @Norelease TODO: Check that it is a valid type.
bool read_RCB_Packet_Type(RCB_Packet_Type *_type, Network_Node *node)
{
    u64 i;
    if(!read_u64(&i, node)) return false;
    *_type = (RCB_Packet_Type)i;
    return true;
}

bool write_RCB_Packet_Type(RCB_Packet_Type type, Network_Node *node)
{
    return write_u64(type, node);
}


bool read_RCB_Packet_Header(RCB_Packet_Header *_header, Network_Node *node)
{
    Zero(*_header);
    
    Read_To_Ptr(RCB_Packet_Type, &_header->type, node);
    //--

    switch(_header->type) {
        case RCB_HELLO: {
            auto &p = _header->hello;
            
            Read_To_Ptr(Room_Connect_Status, &p.connect_status, node);
        } break;
            
        case RCB_GOODBYE: {
        } break;
            
        case RCB_ROOM_INIT: {
            auto &p = _header->room_init;

            Read_To_Ptr(World_Time, &p.time, node);
            
            Read_To_Ptr(u32, &p.num_entities, node);
            Fail_If_True(p.num_entities > MAX_ENTITIES_PER_ROOM);

            static_assert(sizeof(Tile)    == 1);
            static_assert(sizeof(*p.tiles) == 1);
            Data_Ptr(p.tiles, room_size_x * room_size_y, node);
          
            static_assert(sizeof(p.walk_map.nodes[0]) == 1);
            static_assert(ARRLEN(p.walk_map.nodes) == room_size_x * room_size_y);
            Data_Ptr(p.walk_map.nodes, room_size_x * room_size_y, node);
            
        } break;
            
        case RCB_ROOM_UPDATE: {
            auto &p = _header->room_update;

            Read_To_Ptr(u32, &p.tile0, node);
            Read_To_Ptr(u32, &p.tile1, node);
            Fail_If_True(p.tile0 >= room_size_x * room_size_y);
            Fail_If_True(p.tile1 >  room_size_x * room_size_y);
            Fail_If_True(p.tile1 - p.tile0 > room_size_x * room_size_y);
            
            Read_To_Ptr(u32, &p.num_entities, node);
            Fail_If_True(p.num_entities > MAX_ENTITIES_PER_ROOM);

            Read_To_Ptr(u16, &p.num_chat_messages, node);
            Fail_If_True(p.num_chat_messages > MAX_CHAT_MESSAGES_PER_ROOM);

            static_assert(sizeof(Tile)    == 1);
            static_assert(sizeof(*p.tiles) == 1);
            Data_Ptr(p.tiles, p.tile1 - p.tile0, node);

            static_assert(sizeof(p.walk_map.nodes[0]) == 1);
            static_assert(ARRLEN(p.walk_map.nodes) == room_size_x * room_size_y);
            Data_Ptr(p.walk_map.nodes, room_size_x * room_size_y, node);
        } break;
    }

    return true;
}


#ifdef RCB_OUTBOUND

bool enqueue_RCB_HELLO_packet(Network_Node *node, Room_Connect_Status connect_status)
{
    begin_outbound_packet(node);
    {
        Write(RCB_Packet_Type, RCB_HELLO, node);
        // --

        Write(Room_Connect_Status, connect_status, node);
    }
    end_outbound_packet(node);
    return true;
};

bool send_RCB_HELLO_packet_now(Network_Node *node, Room_Connect_Status connect_status)
{
    Send_Now(RCB_HELLO, node, connect_status);
    return true;
}

bool enqueue_RCB_GOODBYE_packet(Network_Node *node)
{
    begin_outbound_packet(node);
    {
        Write(RCB_Packet_Type, RCB_GOODBYE, node);
        // --
    }
    end_outbound_packet(node);
    return true;
}

bool send_RCB_GOODBYE_packet_now(Network_Node *node)
{
    Send_Now_NoArgs(RCB_GOODBYE, node);
    return true;
}

bool enqueue_RCB_ROOM_INIT_packet(Network_Node *node, World_Time time, u32 num_entities, Entity *entities, Tile *tiles, Walk_Map *walk_map)
{
    begin_outbound_packet(node);
    {
        Write(RCB_Packet_Type, RCB_ROOM_INIT, node);
        // --

        Write(World_Time, time, node);

        Write(u32, num_entities, node);
    
        Assert(sizeof(Tile) == 1);
        static_assert(sizeof(*RCB_Packet_Header::room_init.tiles) == 1);
        Write_Bytes(tiles, room_size_x * room_size_y, node);

        static_assert(ARRLEN(RCB_Packet_Header::room_init.walk_map.nodes) == room_size_x * room_size_y);
        static_assert(sizeof(RCB_Packet_Header::room_init.walk_map.nodes[0]) == 1);
        Write_Bytes(walk_map->nodes, room_size_x * room_size_y, node);
                
        for(int i = 0; i < num_entities; i++) {
            Write(Entity, &entities[i], node);
        }
    }
    end_outbound_packet(node);
    return true;
}

// NOTE: tiles should point to all_tiles + tile0
bool enqueue_RCB_ROOM_UPDATE_packet(Network_Node *node,
                                    u32 tile0, u32 tile1, Tile *tiles, Walk_Map *walk_map,
                                    u32 num_entities, Entity *entities,
                                    u16 num_chat_messages, Chat_Message *chat_messages)
{
    begin_outbound_packet(node);
    {
        Write(RCB_Packet_Type, RCB_ROOM_UPDATE, node);
        //--
        
        Write(u32, tile0, node);
        Write(u32, tile1, node);
        Write(u32, num_entities,      node);
        Write(u16, num_chat_messages, node);

        static_assert(sizeof(Tile) == 1);
        static_assert(sizeof(*RCB_Packet_Header::room_update.tiles) == 1);
        Write_Bytes(tiles, (tile1 - tile0), node);

        static_assert(ARRLEN(RCB_Packet_Header::room_update.walk_map.nodes) == room_size_x * room_size_y);
        static_assert(sizeof(RCB_Packet_Header::room_update.walk_map.nodes[0]) == 1);
        Write_Bytes(walk_map->nodes, room_size_x * room_size_y, node);
        

        Entity *at_entity  = entities;
        Entity *end_entity = entities + num_entities;
        while(at_entity < end_entity) {
            Write(Entity, at_entity++, node);
        }
        
        Chat_Message *at_chat  = chat_messages;
        Chat_Message *end_chat = chat_messages + num_chat_messages;
        while(at_chat < end_chat) {
            Write(Chat_Message, *(at_chat++), node);
        }
    }
    end_outbound_packet(node);
    return true;
}

#endif // RCB_OUTBOUND

#endif  // ROOM_CLIENT_BOUND_INCLUDED
