
// IMPORTANT: There is one definition for this for the client, and another for the server.
#define Fail_If_True(Condition) \
    if(Condition) { Debug_Print("[FAILURE] Condition met: "); Debug_Print(#Condition); Debug_Print("\n"); return false; }


const int LISTENING_SOCKET_BACKLOG_SIZE = 1024;


struct Server;

struct Room_Client {
    Socket sock;

    Server *server;
    Room_ID room;
};

void clear(Room_Client *client) {

}

struct Room_Client_Queue
{
    Mutex mutex;
    
    int num_clients;
    Room_Client clients[32];
    Room_ID     rooms[32];
};

// @Temporary?
template<typename T>
struct Atomic {
    Mutex mutex;
    T value;
};

template<typename T>
void init_atomic(Atomic<T> *a)
{
    create_mutex(a->mutex);
}

template<typename T>
void deinit_atomic(Atomic<T> *a)
{
    delete_mutex(a->mutex);
}

template<typename T>
T get(Atomic<T> *a) {
    T result;
    lock_mutex(a->mutex);
    {
        result = a->value;
    }
    unlock_mutex(a->mutex);
    return result;
}

template<typename T>
void set(Atomic<T> *a, T value) {
    lock_mutex(a->mutex);
    {
        a->value = value;
    }
    unlock_mutex(a->mutex);
}

struct Server {
    Atomic<bool> listening_loop_client_accept_failed;

    Array<Room_ID, ALLOC_GAME> room_ids;
    Array<Room, ALLOC_GAME> rooms;
    
    Array<Array<Room_Client, ALLOC_APP>, ALLOC_APP> room_clients;
    Room_Client_Queue room_client_queue;

    Socket listening_socket;
};
