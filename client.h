
enum Room_Server_Connection_Status
{
    ROOM_SERVER_DISCONNECTED,
    ROOM_SERVER_CONNECTED,
};

struct Room_Server_Connection
{
    Room_Server_Connection_Status status;
    Room_ID current_room;
    Socket socket;

    bool last_connect_attempt_failed;
};
bool equal(Room_Server_Connection *a, Room_Server_Connection *b)
{
    if(a->status != b->status) return false;
    if(a->current_room != b->current_room) return false;
    if(!equal(&a->socket, &b->socket)) return false;
    
    if(a->last_connect_attempt_failed != b->last_connect_attempt_failed) return false;

    return true;
}

typedef Array<u8, ALLOC_NETWORK> Packet_Queue;

struct Server_Connections
{
    // ROOM SERVER //
    bool room_connect_requested; // Writable by main loop
    Room_ID requested_room;      // Writable by main loop
    Packet_Queue rsb_queue;      // Writable by main loop
    Room_Server_Connection room;
    // //// ////// //    
};





// @Cleanup: Move this.


#define Enqueue(Type, Dest, Queue_Ptr) \
    enqueue_##Type(Dest, Queue_Ptr)

inline
void enqueue(void *data, u64 length, Packet_Queue *queue)
{
    array_add(*queue, (u8 *)data, length);
}

// @NoRelease
// TODO @Incomplete: On big-endian machines, transform to/from little-endian on send/receive.
//                   (We DO NOT use the 'network byte order' which is big-endian).
//                   This is for @Speed. Most machines we are targeting are little-endian.


inline
void enqueue_s8(s8 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_s16(s16 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_s32(s32 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_s64(s64 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_u8(u8 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_u16(u16 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_u32(u32 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_u64(u64 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

void enqueue_RSB_Packet_Header(RSB_Packet_Type type, Packet_Queue *queue)
{
    Enqueue(u64, type, queue);
}

void enqueue_rsb_Click_Tile_packet(Packet_Queue *queue, u64 tile_ix)
{
    Enqueue(RSB_Packet_Header, RSB_CLICK_TILE, queue);

    Enqueue(u64, tile_ix, queue);
}



    
struct Client
{
    Layout_Manager layout;
    UI_Manager ui;
    Input_Manager input;

    Window main_window;
    Rect main_window_a;

    Font fonts[NUM_FONTS] = {0};

    // NETWORKING //
    Server_Connections server_connections;
    // --
    
    // --
    Game game;
    // --
    
    // @Norelease: Doing Developer stuff in release build...
    Developer developer;
};
