

struct Room_Server;

struct RS_Client {
    Network_Node node;

    Room_Server *server;
    Room_ID room;
    User_ID user; // Can be NO_USER if the client is another server.

    World_Time room_t_on_connect;
};

void clear(RS_Client *client) {

}

struct RS_Client_Queue
{
    Mutex mutex;
    
    int count;
    RS_Client clients[32];
};

// NOTE: Assumes we've already zeroed queue.
void init_room_client_queue(RS_Client_Queue *queue)
{
    create_mutex(queue->mutex);
}

void deinit_room_client_queue(RS_Client_Queue *queue)
{
    delete_mutex(queue->mutex);
}


struct RS_User_Server_Connection
{
    User_ID      user_id;
    Network_Node node;
};

struct Room_Server
{
    u32 server_id;
    
    Atomic<bool> should_exit; // @Speed: Semaphore?

    Listening_Loop listening_loop;

    u64 next_item_number;

    Array<Room_ID, ALLOC_GAME> room_ids;
    Array<Room, ALLOC_GAME> rooms;
    
    Array<Array<RS_Client, ALLOC_APP>, ALLOC_APP> clients;
    RS_Client_Queue client_queue;

    Array<RS_User_Server_Connection, ALLOC_APP> user_server_connections;
};

void init_room_server(Room_Server *server, u32 server_id)
{
    init_atomic(&server->should_exit);

    init_room_client_queue(&server->client_queue);

    server->server_id = server_id;
}

// NOTE: Does not dealloc memory etc. This is just to release os handles etc.
//       Restarting the room server over and over again is not guaranteed not to involve memory leaks or... bugs.
void deinit_room_server(Room_Server *server)
{
    deinit_atomic(&server->should_exit);

    deinit_room_client_queue(&server->client_queue);
}
