
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
