
// USER SERVER/CLIENT PACKETS //

// IMPORTANT: Must fit in a u64
enum User_Connect_Status
{
    USER_CONNECT__NO_STATUS = 0, // This is used to indicate that we failed to read a status.

    USER_CONNECT__CONNECTED              = 1,
    USER_CONNECT__INCORRECT_CREDENTIALS  = 2

};

enum UCB_Packet_Type
{
    UCB_GOODBYE = 1,
    UCB_USER_INIT = 2,
};

// User Client Bound Packet Header
struct UCB_Packet_Header
{
    UCB_Packet_Type type;
};

enum USB_Packet_Type
{
    USB_GOODBYE = 1,
};

// User Server Bound Packet Header
struct USB_Packet_Header
{
    USB_Packet_Type type;
};


#define Write_USB_Header(Packet_Type, Sock_Ptr)    \
    Fail_If_True(!write_USB_Packet_Header({Packet_Type}, Sock_Ptr))

#define Write_UCB_Header(Packet_Type, Sock_Ptr)    \
    Fail_If_True(!write_UCB_Packet_Header({Packet_Type}, Sock_Ptr))



inline
User_Connect_Status read_user_connect_status_code(Socket *sock)
{
    u64 i;
    bool result = read_u64(&i, sock);
    if(!result) return USER_CONNECT__NO_STATUS;

    return (User_Connect_Status)i;
}

inline
bool write_user_connect_status_code(User_Connect_Status status, Socket *sock)
{
    return write_u64(status, sock);
}







inline
bool read_USB_Packet_Type(USB_Packet_Type *_type, Socket *sock)
{
    u64 i;
    if(!read_u64(&i, sock)) return false;
    *_type = (USB_Packet_Type)i;
    return true;
}

inline
bool write_USB_Packet_Type(USB_Packet_Type type, Socket *sock)
{
    return write_u64(type, sock);
}


inline
bool read_USB_Packet_Header(USB_Packet_Header *_header, Socket *sock)
{
    Zero(*_header);
    Read_To_Ptr(USB_Packet_Type, &_header->type, sock);
    return true;
}

inline
bool write_USB_Packet_Header(USB_Packet_Header header, Socket *sock)
{
    Write(USB_Packet_Type, header.type, sock);
    return true;
}


// NOTE: tiles should point to all_tiles + tile0
bool write_usb_Goodbye_packet(Socket *sock)
{
    Write_USB_Header(USB_GOODBYE, sock);

    return true;
}









inline
bool read_UCB_Packet_Type(UCB_Packet_Type *_type, Socket *sock)
{
    u64 i;
    if(!read_u64(&i, sock)) return false;
    *_type = (UCB_Packet_Type)i;
    return true;
}

inline
bool write_UCB_Packet_Type(UCB_Packet_Type type, Socket *sock)
{
    return write_u64(type, sock);
}



inline
bool read_UCB_Packet_Header(UCB_Packet_Header *_header, Socket *sock)
{
    Zero(*_header);
    Read_To_Ptr(UCB_Packet_Type, &_header->type, sock);
    return true;
}

inline
bool write_UCB_Packet_Header(UCB_Packet_Header header, Socket *sock)
{
    Write(UCB_Packet_Type, header.type, sock);
    return true;
}



bool write_ucb_Goodbye_packet(Socket *sock)
{
    Write_UCB_Header(UCB_GOODBYE, sock);

    return true;
}


bool read_ucb_User_Init_header(Socket *sock, String *_username, v4 *_color)
{
    Read_To_Ptr(String, _username, sock, ALLOC_TMP);
    Read_To_Ptr(v4, _color, sock);

    return true;
}

bool write_ucb_User_Init_packet(Socket *sock, String username, v4 color)
{
    Write_UCB_Header(UCB_USER_INIT, sock);
    
    Write(String, username, sock);
    Write(v4, color, sock);

    return true;
}
