

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

// NOTE: Assumes we've already zeroed queue.
void init_room_client_queue(Room_Client_Queue *queue)
{
    create_mutex(queue->mutex);
}

void deinit_room_client_queue(Room_Client_Queue *queue)
{
    delete_mutex(queue->mutex);
}


struct Room_Server {
    Atomic<bool> should_exit; // @Speed: Semaphore?

    Listening_Loop listening_loop;

    Array<Room_ID, ALLOC_GAME> room_ids;
    Array<Room, ALLOC_GAME> rooms;
    
    Array<Array<Room_Client, ALLOC_APP>, ALLOC_APP> clients;
    Room_Client_Queue client_queue;
};

void init_room_server(Room_Server *server)
{
    init_atomic(&server->should_exit);

    init_room_client_queue(&server->client_queue);
}

// NOTE: Does not dealloc memory etc. This is just to release os handles etc.
//       Restarting the room server over and over again is not guaranteed not to involve memory leaks or... bugs.
void deinit_room_server(Room_Server *server)
{
    deinit_atomic(&server->should_exit);

    deinit_room_client_queue(&server->client_queue);
}
