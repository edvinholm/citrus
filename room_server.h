

struct Room_Server;

struct Room_Client {
    Socket sock;

    Room_Server *server;
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

struct Room_Server {
    Atomic<bool> should_exit; // @Speed: Semaphore?

    Atomic<bool> listening_loop_should_exit;
    Atomic<bool> listening_loop_client_accept_failed; // @Speed: Semaphore?

    Array<Room_ID, ALLOC_GAME> room_ids;
    Array<Room, ALLOC_GAME> rooms;
    
    Array<Array<Room_Client, ALLOC_APP>, ALLOC_APP> clients;
    Room_Client_Queue client_queue;
    
    Socket listening_socket;
};
