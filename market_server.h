
#include "market_server_bound.h"

struct Market_Server;

struct MS_Client
{
    MS_Client_Type type;
    
    Network_Node node;

    User_ID user_id;
    Market_Server *server;
};

void clear(MS_Client *client)
{
    
}

struct MS_Client_Queue
{
    Mutex mutex;
    
    int num_clients;
    MS_Client clients[32];
};

// NOTE: Assumes we've already zeroed queue.
void init_user_client_queue(MS_Client_Queue *queue)
{
    create_mutex(queue->mutex);
}

void deinit_user_client_queue(MS_Client_Queue *queue)
{
    delete_mutex(queue->mutex);
}


struct Market_Server
{
    u32 server_id;
    
    Atomic<bool> should_exit; // @Speed: Semaphore?

    Listening_Loop listening_loop;
    
    Array<MS_Client, ALLOC_APP> clients; // IMPORTANT: Must map 1:1 to users.
    MS_Client_Queue client_queue;
};


void init_market_server(Market_Server *server, u32 server_id)
{
    init_atomic(&server->should_exit);
    
    init_user_client_queue(&server->client_queue);

    server->server_id = server_id;
}

// NOTE: Does not dealloc memory etc. This is just to release os handles etc.
//       Restarting the user server over and over again is not guaranteed not to involve memory leaks or... bugs.
void deinit_market_server(Market_Server *server)
{
    deinit_atomic(&server->should_exit);

    deinit_user_client_queue(&server->client_queue);
}


