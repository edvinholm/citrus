
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
    RCB_TILES_CHANGED = 2
};

// Room Client Bound Packet Header
struct RCB_Packet_Header
{
    RCB_Packet_Type type;
};

enum RSB_Packet_Type
{
    RSB_GOODBYE = 1
};

// Room Server Bound Packet Header
struct RSB_Packet_Header
{
    RSB_Packet_Type type;
};



// @NoRelease
// TODO @Incomplete: On big-endian machines, transform to/from little-endian on send/receive.
//                   (We DO NOT use the 'network byte order' which is big-endian).
//                   This is for @Speed. Most machines we are targeting are little-endian.


inline
bool read_from_socket(void *_data, u64 length, Socket *sock)
{
    return platform_read_from_socket((u8 *)_data, length, sock);
}


inline
bool read_s8(s8 *_i, Socket *sock)
{
    return read_from_socket(_i, sizeof(*_i), sock);
}

inline
bool read_s16(s16 *_i, Socket *sock)
{
    return read_from_socket(_i, sizeof(*_i), sock);
}

inline
bool read_s32(s32 *_i, Socket *sock)
{
    return read_from_socket(_i, sizeof(*_i), sock);
}

inline
bool read_s64(s64 *_i, Socket *sock)
{
    return read_from_socket(_i, sizeof(*_i), sock);
}


inline
bool read_u8(u8 *_i, Socket *sock)
{
    return read_from_socket(_i, sizeof(*_i), sock);
}

inline
bool read_u16(u16 *_i, Socket *sock)
{
    return read_from_socket(_i, sizeof(*_i), sock);
}

inline
bool read_u32(u32 *_i, Socket *sock)
{
    return read_from_socket(_i, sizeof(*_i), sock);
}

inline
bool read_u64(u64 *_i, Socket *sock)
{
    return read_from_socket(_i, sizeof(*_i), sock);
}



inline
bool write_to_socket(void *data, u64 length, Socket *sock)
{
    return platform_write_to_socket((u8 *)data, length, sock);
}


inline
bool write_s8(s8 i, Socket *sock)
{
    return write_to_socket(&i, sizeof(i), sock);
}

inline
bool write_s16(s16 i, Socket *sock)
{
    return write_to_socket(&i, sizeof(i), sock);
}

inline
bool write_s32(s32 i, Socket *sock)
{
    return write_to_socket(&i, sizeof(i), sock);
}

inline
bool write_s64(s64 i, Socket *sock)
{
    return write_to_socket(&i, sizeof(i), sock);
}



inline
bool write_u8(u8 i, Socket *sock)
{
    return write_to_socket(&i, sizeof(i), sock);
}

inline
bool write_u16(u16 i, Socket *sock)
{
    return write_to_socket(&i, sizeof(i), sock);
}

inline
bool write_u32(u32 i, Socket *sock)
{
    return write_to_socket(&i, sizeof(i), sock);
}

inline
bool write_u64(u64 i, Socket *sock)
{
    return write_to_socket(&i, sizeof(i), sock);
}

#define Read_Bytes(Dest, Length, Sock_Ptr)      \
    Fail_If_True(!read_from_socket(Dest, Length, Sock_Ptr))

#define Write_Bytes(Src, Length, Sock_Ptr)      \
    Fail_If_True(!write_to_socket(Src, Length, Sock_Ptr))

#define Read(Type, Dest, Sock_Ptr) \
    Fail_If_True(!read_##Type(Dest, Sock_Ptr))

#define Write(Type, Dest, Sock_Ptr) \
    Fail_If_True(!write_##Type(Dest, Sock_Ptr))


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
    Read(RSB_Packet_Type, &_header->type, sock);
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
    RSB_Packet_Header header = {0};
    header.type = RSB_GOODBYE;
    
    Write(RSB_Packet_Header, header, sock);

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
    Read(RCB_Packet_Type, &_header->type, sock);
    return true;
}

inline
bool write_RCB_Packet_Header(RCB_Packet_Header header, Socket *sock)
{
    Write(RCB_Packet_Type, header.type, sock);
    return true;
}


// NOTE: tiles should point to all_tiles + tile0
bool write_rcb_Goodbye_packet(Socket *sock)
{
    RCB_Packet_Header header = {0};
    header.type = RCB_GOODBYE;
    
    Write(RCB_Packet_Header, header, sock);

    return true;
}


// NOTE: tiles should point to all_tiles + tile0
bool write_rcb_Tiles_Changed_packet(Socket *sock,
                                    u64 tile0, u64 tile1, Tile *tiles)
{
    RCB_Packet_Header header = {0};
    header.type = RCB_TILES_CHANGED;
    
    Write(RCB_Packet_Header, header, sock);

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


bool read_rcb_Tiles_Changed_header(Socket *sock,
                                  u64 *_tile0, u64 *_tile1)
{
    Read(u64, _tile0, sock);
    Read(u64, _tile1, sock);

    return true;
}
