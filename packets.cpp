

// @NoRelease
// TODO @Incomplete: On big-endian machines, transform to/from little-endian on send/receive.
//                   (We DO NOT use the 'network byte order' which is big-endian).
//                   This is for @Speed. Most machines we are targeting are little-endian.


#define Read_Bytes(Dest, Length, Sock_Ptr)      \
    Fail_If_True(!read_from_socket(Dest, Length, Sock_Ptr))

#define Write_Bytes(Src, Length, Sock_Ptr)      \
    Fail_If_True(!write_to_socket(Src, Length, Sock_Ptr))

#define Read_To_Ptr(Type, Dest, ...) \
    Fail_If_True(!read_##Type(Dest, __VA_ARGS__))

#define Read(Type, Ident, ...) \
    Type Ident;                                 \
    Fail_If_True(!read_##Type(&Ident, __VA_ARGS__))

#define Write(Type, Val, Sock_Ptr) \
    Fail_If_True(!write_##Type(Val, Sock_Ptr))



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
bool read_v3(v3 *_u, Socket *sock)
{
    Assert(sizeof(_u->x) == sizeof(u32));
    Assert(sizeof(_u->x) == 4);
    u32 x, y, z;    
    if(!read_u32(&x, sock)) return false;
    if(!read_u32(&y, sock)) return false;
    if(!read_u32(&z, sock)) return false;

    memcpy(&_u->x, &x, 4);
    memcpy(&_u->y, &y, 4);
    memcpy(&_u->z, &z, 4);

    return true;
}

inline
bool read_v4(v4 *_u, Socket *sock)
{
    Assert(sizeof(_u->x) == sizeof(u32));
    Assert(sizeof(_u->x) == 4);
    u32 x, y, z, w;    
    if(!read_u32(&x, sock)) return false;
    if(!read_u32(&y, sock)) return false;
    if(!read_u32(&z, sock)) return false;
    if(!read_u32(&w, sock)) return false;

    memcpy(&_u->x, &x, 4);
    memcpy(&_u->y, &y, 4);
    memcpy(&_u->z, &z, 4);
    memcpy(&_u->w, &w, 4);

    return true;
}

inline
bool read_String(String *_str, Socket *sock, Allocator_ID allocator)
{
    u64 length;
    if(!read_u64(&length, sock)) return false;
    _str->length = length;
    _str->data = alloc(length, allocator); // @Norelease: TODO @Security: Should have a max length for strings. If the client sends a huge number here, we would try to alloc it and fail and crash and..........
    if(!read_from_socket(_str->data, _str->length, sock)) {
        dealloc_if_legal(_str->data, allocator);
        return false;
    }
    return true;
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

inline
bool write_float(float f, Socket *sock)
{
    Assert(sizeof(f) == sizeof(u32));
    Assert(sizeof(f) == 4);
    u32 i;
    memcpy(&i, &f, 4);
    return write_u32(i, sock);
}

inline
bool write_v3(v3 u, Socket *sock)
{
    Assert(sizeof(u.x) == sizeof(u32));
    Assert(sizeof(u.x) == 4);
    u32 x, y, z, w;
    memcpy(&x, &u.x, 4);
    memcpy(&y, &u.y, 4);
    memcpy(&z, &u.z, 4);

    if(!write_u32(x, sock)) return false;
    if(!write_u32(y, sock)) return false;
    if(!write_u32(z, sock)) return false;

    return true;
}


inline
bool write_v4(v4 u, Socket *sock)
{
    Assert(sizeof(u.x) == sizeof(u32));
    Assert(sizeof(u.x) == 4);
    u32 x, y, z, w;
    memcpy(&x, &u.x, 4);
    memcpy(&y, &u.y, 4);
    memcpy(&z, &u.z, 4);
    memcpy(&w, &u.w, 4);

    if(!write_u32(x, sock)) return false;
    if(!write_u32(y, sock)) return false;
    if(!write_u32(z, sock)) return false;
    if(!write_u32(w, sock)) return false;

    return true;
}

inline
bool write_String(String str, Socket *sock)
{
    // @Norelease: TODO @Security: Assert str.length < max length for strings... See note in read_String.
    if(!write_u64(str.length, sock)) return false;
    if(!write_to_socket(str.data, str.length, sock)) return false;

    return true;
}






// Item //
bool read_Item_Type_ID(Item_Type_ID *_type_id, Socket *sock)
{
    Read(u64, type_id, sock);
    *_type_id = (Item_Type_ID)type_id;
    return true;
}

bool write_Item_Type_ID(Item_Type_ID type_id, Socket *sock)
{
    Write(u64, type_id, sock);
    return true;
}



#include "packets_room.cpp"
#include "packets_user.cpp"
