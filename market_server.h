
// @Norelease @Bug: Selling for $0 does not work.

struct Market_Server;

typedef s64 MS_Client_ID;

struct MS_Client
{
    MS_Client_ID id;
    MS_Client_Type type;
    
    Network_Node node;

    User_ID      user_id;
    
    Market_View_Target view_target;
    
    Market_Server *server;
};

void clear(MS_Client *client)
{
    
}

struct MS_Client_Queue
{
    Mutex mutex;
    
    MS_Client_ID next_client_id;
    
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

struct Market_Order
{
    bool is_buy_order;
    Money price;

    User_ID user;

    union {
        struct {
            u32 reserved_slot_ix; // Where in the buyer's inventory the item will end up.
        } buy;

        struct {
            Item item;
        } sell;
    };
};

struct Market_Article
{
    Array<Market_Order, ALLOC_MALLOC> orders;
    Array<MS_Client_ID, ALLOC_MALLOC> watchers;

    u16 price_history_length; // @Norelease: Save minute, hour, day, month, year. (Later our own time units).
    Money price_history[128];
    
    bool did_change;
};


struct MS_User_Server_Connection
{
    User_ID user_id;
    Network_Node node;
};

struct Market_Server
{
    Thread thread;
    u32 server_id;
    
    Atomic<bool> should_exit; // @Speed: Semaphore?

    Listening_Loop listening_loop;

    Market_Article articles[ITEM_NONE_OR_NUM];
    
    Array<MS_Client, ALLOC_MALLOC> clients;
    MS_Client_Queue client_queue;

    Array<MS_User_Server_Connection, ALLOC_MALLOC> user_server_connections;
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


