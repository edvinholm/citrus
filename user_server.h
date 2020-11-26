
struct User_Server;

struct User_Client
{
    Socket sock;

    String username; // @Temporary: Should be an ID if anything
    User_Server *server;
};

void clear(User_Client *client)
{
    clear(&client->username, ALLOC_US);
}


struct User_Client_Queue
{
    Mutex mutex;
    
    int num_clients;
    User_Client clients[32];
};

// NOTE: Assumes we've already zeroed queue.
void init_user_client_queue(User_Client_Queue *queue)
{
    create_mutex(queue->mutex);
}

void deinit_user_client_queue(User_Client_Queue *queue)
{
    delete_mutex(queue->mutex);
}


struct User_Server
{
    Atomic<bool> should_exit; // @Speed: Semaphore?

    Array<User, ALLOC_US> users;

    Listening_Loop listening_loop;
    
    Array<Array<User_Client, ALLOC_APP>, ALLOC_APP> clients; // IMPORTANT: Must map 1:1 to users.
    User_Client_Queue client_queue;
};

void init_user_server(User_Server *server)
{
    init_atomic(&server->should_exit);
    
    init_user_client_queue(&server->client_queue);
}

// NOTE: Does not dealloc memory etc. This is just to release os handles etc.
//       Restarting the user server over and over again is not guaranteed not to involve memory leaks or... bugs.
void deinit_user_server(User_Server *server)
{
    deinit_atomic(&server->should_exit);

    deinit_user_client_queue(&server->client_queue);
}

