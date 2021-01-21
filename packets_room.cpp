
// ROOM SERVER/CLIENT PACKETS //

// IMPORTANT: Must fit in a u64
enum Room_Connect_Status
{
    ROOM_CONNECT__NO_STATUS = 0, // This is used to indicate that we failed to read a status.

    
    ROOM_CONNECT__REQUEST_RECEIVED = 1,
    ROOM_CONNECT__CONNECTED        = 2,
    
    ROOM_CONNECT__INVALID_ROOM_ID  = 3

};

enum RCB_Packet_Type
{
    RCB_GOODBYE   = 1,
    RCB_ROOM_INIT = 2,
    
    RCB_ROOM_CHANGED = 3,
};

// Room Client Bound Packet Header
struct RCB_Packet_Header
{
    RCB_Packet_Type type;
};

enum RSB_Packet_Type
{
    RSB_GOODBYE = 1,

    RSB_CLICK_TILE = 2,
};

// Room Server Bound Packet Header
struct RSB_Packet_Header
{
    RSB_Packet_Type type;
};


#define Write_RSB_Header(Packet_Type, Sock_Ptr)    \
    Fail_If_True(!write_RSB_Packet_Header({Packet_Type}, Sock_Ptr))

#define Write_RCB_Header(Packet_Type, Sock_Ptr)    \
    Fail_If_True(!write_RCB_Packet_Header({Packet_Type}, Sock_Ptr))



Room_Connect_Status read_room_connect_status_code(Socket *sock)
{
    u64 i;
    bool result = read_u64(&i, sock);
    if(!result) return ROOM_CONNECT__NO_STATUS;

    return (Room_Connect_Status)i;
}


bool write_room_connect_status_code(Room_Connect_Status status, Socket *sock)
{
    return write_u64(status, sock);
}



// Tile //
bool read_Tile(Tile *_tile, Socket *sock)
{
    Read(u8, i, sock);
    *_tile = (Tile)i;
    return true;
}

bool write_Tile(Tile tile, Socket *sock)
{
    return write_u8(tile, sock);
}



// Entity //
bool read_Entity_Type(Entity_Type *_type, Socket *sock)
{
    Read(u8, type, sock);
    *_type = (Entity_Type)type;
    return true;
}

bool write_Entity_Type(Entity_Type type, Socket *sock)
{
    Write(u8, type, sock);
    return true;
}

bool read_Entity(Entity *_entity, Socket *sock)
{
    S__Entity *shared = &_entity->shared;

    Read_To_Ptr(v3,          &shared->p,    sock);
    Read_To_Ptr(Entity_Type, &shared->type, sock);

    switch(shared->type) {
        case ENTITY_ITEM: {
            Read_To_Ptr(Item_Type_ID, &shared->item_type, sock);
        } break;

        default: Assert(false); return false;
    }
    
    return true;
}

bool write_Entity(Entity *entity, Socket *sock)
{
    S__Entity *shared = &entity->shared;

    Write(v3, shared->p,    sock);
    Write(Entity_Type, shared->type, sock);

    switch(shared->type) {
        case ENTITY_ITEM: {
            Write(Item_Type_ID, shared->item_type, sock);
        } break;

        default: Assert(false); return false;
    }

    return true;
}




bool read_RSB_Packet_Type(RSB_Packet_Type *_type, Socket *sock)
{
    u64 i;
    if(!read_u64(&i, sock)) return false;
    *_type = (RSB_Packet_Type)i;
    return true;
}


bool write_RSB_Packet_Type(RSB_Packet_Type type, Socket *sock)
{
    return write_u64(type, sock);
}



bool read_RSB_Packet_Header(RSB_Packet_Header *_header, Socket *sock)
{
    Zero(*_header);
    Read_To_Ptr(RSB_Packet_Type, &_header->type, sock);
    return true;
}


bool write_RSB_Packet_Header(RSB_Packet_Header header, Socket *sock)
{
    Write(RSB_Packet_Type, header.type, sock);
    return true;
}


// NOTE: tiles should point to all_tiles + tile0
bool write_rsb_Goodbye_packet(Socket *sock)
{
    Write_RSB_Header(RSB_GOODBYE, sock);

    return true;
}





bool read_RCB_Packet_Type(RCB_Packet_Type *_type, Socket *sock)
{
    u64 i;
    if(!read_u64(&i, sock)) return false;
    *_type = (RCB_Packet_Type)i;
    return true;
}


bool write_RCB_Packet_Type(RCB_Packet_Type type, Socket *sock)
{
    return write_u64(type, sock);
}



bool read_RCB_Packet_Header(RCB_Packet_Header *_header, Socket *sock)
{
    Zero(*_header);
    Read_To_Ptr(RCB_Packet_Type, &_header->type, sock);
    return true;
}


bool write_RCB_Packet_Header(RCB_Packet_Header header, Socket *sock)
{
    Write(RCB_Packet_Type, header.type, sock);
    return true;
}


bool write_rcb_Goodbye_packet(Socket *sock)
{
    Write_RCB_Header(RCB_GOODBYE, sock);

    return true;
}


// NOTE: tiles should point to all_tiles + tile0
bool write_rcb_Room_Changed_packet(Socket *sock,
                                   u64 tile0, u64 tile1, Tile *tiles,
                                   u64 num_entities, Entity *entities)
{
    Write_RCB_Header(RCB_ROOM_CHANGED, sock);

    /* Header */
    Write(u64, tile0, sock);
    Write(u64, tile1, sock);
    Write(u64, num_entities, sock);
    /* ------ */

#if 0
    Tile *at  = tiles;
    Tile *end = tiles + (tile1 - tile0);

    while(at < end) {
        Write(Tile, *at++, sock);
    }
#else
    Assert(sizeof(Tile) == 1);
    Write_Bytes(tiles, (tile1 - tile0), sock);
#endif

    Entity *at_entity  = entities;
    Entity *end_entity = entities + num_entities;
    while(at_entity < end_entity)
    {
        Write(Entity, at_entity++, sock);
    }

    return true;
}


bool write_rcb_Room_Init_packet(Socket *sock, Tile *tiles, u64 num_entities, Entity *entities)
{
    Write_RCB_Header(RCB_ROOM_INIT, sock);

    Write(u64, num_entities, sock);
    
    Assert(sizeof(Tile) == 1);
    Write_Bytes(tiles, room_size_x * room_size_y, sock);

    for(u64 i = 0; i < num_entities; i++) {
        Write(Entity, &entities[i], sock);
    }

    return true;
}


bool read_rcb_Room_Changed_header(Socket *sock,
                                  u64 *_tile0, u64 *_tile1, u64 *_num_entities)
{
    Read_To_Ptr(u64, _tile0,        sock);
    Read_To_Ptr(u64, _tile1,        sock);
    Read_To_Ptr(u64, _num_entities, sock);

    return true;
}
