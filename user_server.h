
#include "user_server_bound.h"

struct User_Server;

struct US_Client
{
    US_Client_Type type;

    Network_Node node;

    User_ID user_id;
    
    bool current_transaction_exists;
    US_Transaction current_transaction;

    u32 server_id; // Only valid if type != US_CLIENT_PLAYER.
    
    User_Server *server;
};

void clear(US_Client *client)
{
}


struct US_Client_Queue
{
    Mutex mutex;
    
    int num_clients;
    US_Client clients[32];
};

// NOTE: Assumes we've already zeroed queue.
void init_user_client_queue(US_Client_Queue *queue)
{
    create_mutex(queue->mutex);
}

void deinit_user_client_queue(US_Client_Queue *queue)
{
    delete_mutex(queue->mutex);
}


struct User_Server
{
    Thread thread;
    u32 server_id;
    
    Atomic<bool> should_exit; // @Speed: Semaphore?

    Array<User, ALLOC_MALLOC> users;

    Listening_Loop listening_loop;
    
    u64 next_item_number;
    
    Array<Array<US_Client, ALLOC_MALLOC>, ALLOC_MALLOC> clients; // IMPORTANT: Must map 1:1 to users.
    US_Client_Queue client_queue;
};



void init_user_server(User_Server *server, u32 server_id)
{
    init_atomic(&server->should_exit);
    
    init_user_client_queue(&server->client_queue);

    server->server_id = server_id;
}

// NOTE: Does not dealloc memory etc. This is just to release os handles etc.
//       Restarting the user server over and over again is not guaranteed not to involve memory leaks or... bugs.
void deinit_user_server(User_Server *server)
{
    deinit_atomic(&server->should_exit);

    deinit_user_client_queue(&server->client_queue);
}

