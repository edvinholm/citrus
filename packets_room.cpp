
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
    RCB_GOODBYE = 1,
    RCB_ROOM_INIT = 3,
    
    RCB_TILES_CHANGED = 2,
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


inline
Room_Connect_Status read_room_connect_status_code(Socket *sock)
{
    u64 i;
    bool result = read_u64(&i, sock);
    if(!result) return ROOM_CONNECT__NO_STATUS;

    return (Room_Connect_Status)i;
}

inline
bool write_room_connect_status_code(Room_Connect_Status status, Socket *sock)
{
    return write_u64(status, sock);
}




inline
bool read_Tile(Tile *_tile, Socket *sock)
{
    u8 i;
    if(!read_u8(&i, sock)) return false;
    *_tile = (Tile)i;
    return true;
}

inline
bool write_Tile(Tile tile, Socket *sock)
{
    return write_u8(tile, sock);
}




inline
bool read_RSB_Packet_Type(RSB_Packet_Type *_type, Socket *sock)
{
    u64 i;
    if(!read_u64(&i, sock)) return false;
    *_type = (RSB_Packet_Type)i;
    return true;
}

inline
bool write_RSB_Packet_Type(RSB_Packet_Type type, Socket *sock)
{
    return write_u64(type, sock);
}


inline
bool read_RSB_Packet_Header(RSB_Packet_Header *_header, Socket *sock)
{
    Zero(*_header);
    Read_To_Ptr(RSB_Packet_Type, &_header->type, sock);
    return true;
}

inline
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




inline
bool read_RCB_Packet_Type(RCB_Packet_Type *_type, Socket *sock)
{
    u64 i;
    if(!read_u64(&i, sock)) return false;
    *_type = (RCB_Packet_Type)i;
    return true;
}

inline
bool write_RCB_Packet_Type(RCB_Packet_Type type, Socket *sock)
{
    return write_u64(type, sock);
}


inline
bool read_RCB_Packet_Header(RCB_Packet_Header *_header, Socket *sock)
{
    Zero(*_header);
    Read_To_Ptr(RCB_Packet_Type, &_header->type, sock);
    return true;
}

inline
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
bool write_rcb_Tiles_Changed_packet(Socket *sock,
                                    u64 tile0, u64 tile1, Tile *tiles)
{
    Write_RCB_Header(RCB_TILES_CHANGED, sock);

    /* Header */
    Write(u64, tile0, sock);
    Write(u64, tile1, sock);
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

    return true;
}


bool write_rcb_Room_Init_packet(Socket *sock, Tile *tiles)
{
    Write_RCB_Header(RCB_ROOM_INIT, sock);

    Assert(sizeof(Tile) == 1);
    Write_Bytes(tiles, room_size_x * room_size_y, sock);

    return true;
}


bool read_rcb_Tiles_Changed_header(Socket *sock,
                                  u64 *_tile0, u64 *_tile1)
{
    Read_To_Ptr(u64, _tile0, sock);
    Read_To_Ptr(u64, _tile1, sock);

    return true;
}
