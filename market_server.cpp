
#include "market_server_bound.cpp"
#include "market_client_bound.cpp"


const char *LOG_TAG_MS = "$MS$"; // Market Server
const char *LOG_TAG_MS_LIST = "$MS$LIST$"; // Listening loop

#define MS_Log_T(Tag, ...) \
    printf("[%s]", LOG_TAG_MS);                            \
    Log_T(Tag, __VA_ARGS__)
#define MS_Log(...)                     \
    Log_T(LOG_TAG_MS, __VA_ARGS__)
#define MS_Log_No_T(...)                     \
    Log(__VA_ARGS__)


#define MS_LIST_Log_T(Tag, ...) \
    printf("[%s]", LOG_TAG_MS_LIST);      \
    Log_T(Tag, __VA_ARGS__)
#define MS_LIST_Log(...)                     \
    Log_T(LOG_TAG_MS_LIST, __VA_ARGS__)
#define MS_LIST_No_T(...)                     \
    Log(__VA_ARGS__)


void enqueue_client(MS_Client *client, MS_Client_Queue *queue)
{
    lock_mutex(queue->mutex);
    {
        Assert(queue->num_clients <= ARRLEN(queue->clients));
                
        // If the queue is full, wait..
        while(queue->num_clients == ARRLEN(queue->clients)) {
            unlock_mutex(queue->mutex);
            platform_sleep_milliseconds(100);
            lock_mutex(queue->mutex);
        }
                
        Assert(queue->num_clients <= ARRLEN(queue->clients));
                
        int ix = queue->num_clients++;
        queue->clients[ix] = *client;
    }
    unlock_mutex(queue->mutex);
}


bool receive_next_msb_packet(Network_Node *node, MSB_Packet_Header *_packet_header, bool *_error, bool block = false)
{
    if(!receive_next_network_node_packet(node, _error, block)) return false;
    
    *_error = true;
    Read_To_Ptr(MSB_Packet_Header, _packet_header, node);
    *_error = false;
    
    return true;
}

bool expect_type_of_next_msb_packet(MSB_Packet_Type expected_type, Network_Node *node, MSB_Packet_Header *_packet_header)
{
    bool error;
    if(!receive_next_msb_packet(node, _packet_header, &error, true)) return false;
    if(_packet_header->type != expected_type) return false;
    return true;
}

// TODO @Norelease @Robustness @Speed: This thing should have enough information to dismiss clients with invalid user IDs or auth tokens.
DWORD market_server_listening_loop(void *market_server_)
{
    auto *server = (Market_Server *)market_server_;
    auto *queue  = &server->client_queue;

    Listening_Loop *loop = &server->listening_loop;

    MS_LIST_Log("Running.\n");
    
    bool   client_accepted;
    Socket client_socket;
    while(listening_loop_running(loop, true, &client_accepted, &client_socket, LOG_TAG_MS_LIST))
    {
        if(!client_accepted) continue;

        MS_LIST_Log("Client accepted :)\n");

        bool success = true;
        
        if(success && !platform_set_socket_read_timeout(&client_socket, 1000)) {
            MS_LIST_Log("Unable to set read timeout for new client's socket (Last WSA Error: %d).\n", WSAGetLastError());
            success = false;
        }

        Network_Node new_node = {0};
        reset_network_node(&new_node, client_socket);

        MSB_Packet_Header header;
        if(success && !expect_type_of_next_msb_packet(MSB_HELLO, &new_node, &header)) {
            MS_LIST_Log("First packet did not have the expected type MSB_HELLO.\n");
            success = false;
        }

        if(!success)
        {
           if(!platform_close_socket(&client_socket)) {
               MS_LIST_Log("Unable to close new client's socket.\n");
           } else {
               MS_LIST_Log("New client's socket closed successfully.\n");
           }
           continue;
        }

        auto *hello = &header.hello;
        
        // ADD CLIENT TO QUEUE //
        MS_Client client = {0};
        client.type     = hello->client_type;
        client.user_id  = hello->user;
        client.server   = server;
        client.node     = new_node;

        MS_LIST_Log("Requested user: %llu\n", client.user_id);

        enqueue_client(&client, queue);

        platform_sleep_milliseconds(1);
    }

    MS_LIST_Log("Exiting.");
    return 0;
}

inline
bool start_market_server_listening_loop(Market_Server *server, Thread *_thread)
{
    return start_listening_loop(&server->listening_loop, MARKET_SERVER_PORT, &market_server_listening_loop, server, _thread, LOG_TAG_MS_LIST);
}


void disconnect_market_client(MS_Client *client)
{
    auto client_copy = *client;

    // REMEMBER: This is special, so don't do anything fancy here that can put us in an infinite disconnection loop.
    if(!send_MCB_GOODBYE_packet_now(&client->node)) {
        MS_Log("Failed to write MCB Goodbye.\n");
    }
    
    if(!platform_close_socket(&client->node.socket)) {
        MS_Log("Failed to close market client socket.\n");
    }

    auto *server = client->server;
    auto client_index = client - server->clients.e;
    array_ordered_remove(server->clients, client_index);

    // IMPORTANT that we clear the copy here. Otherwise we would clear the client that is placed where our client was in the array.
    clear(&client_copy);
}



// Sends an MCB packet if MS_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// MS_Client_Ptr is set to NULL.
#define MCB_Packet(Packet_Ident, MS_Client_Ptr, ...) \
    if(MS_Client_Ptr) {                                                 \
        bool success = true;                                            \
        if(success && !enqueue_MCB_##Packet_Ident##_packet(&MS_Client_Ptr->node, __VA_ARGS__)) success = false; \
        Assert(MS_Client_Ptr->node.packet_queue.n == 1);                \
        if(success && !send_outbound_packets(&MS_Client_Ptr->node)) success = false; \
        if(!success) {                                                  \
            MS_Log("Client disconnected from market server due to MCB packet failure (socket = %lld, WSA Error: %d).\n", MS_Client_Ptr->node.socket.handle, WSAGetLastError()); \
            disconnect_market_client(MS_Client_Ptr);                \
            MS_Client_Ptr = NULL;                                       \
        }                                                               \
    }


// Receives an MSB packet if MS_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// MS_Client_Ptr is set to NULL.
#define MSB_Header(MS_Client_Ptr, Header_Ptr)           \
    if((MS_Client_Ptr) &&                                             \
       !read_MSB_Packet_Header(Header_Ptr, &MS_Client_Ptr->node)) {    \
        MS_Log("Client disconnected from market server due to MSB header failure (socket = %lld, WSA Error: %d).\n", MS_Client_Ptr->node.socket.handle, WSAGetLastError()); \
        disconnect_market_client(MS_Client_Ptr);                       \
        MS_Client_Ptr = NULL;                                         \
    }                                                               \




void add_new_market_clients(Market_Server *server)
{
    auto *queue = &server->client_queue;
    lock_mutex(queue->mutex);
    {
        Assert(queue->num_clients <= ARRLEN(queue->clients));
        
        for(int c = 0; c < queue->num_clients; c++) {

            MS_Client *client = queue->clients + c;
            User_ID requested_user = client->user_id;

            bool success = true;

            // @Norelease: check that the user exists.
            
            if(success && !send_MCB_HELLO_packet_now(&client->node, MARKET_CONNECT__CONNECTED))
            {
                MS_Log("Failed to write MARKET_CONNECT__CONNECTED to client (socket = %lld).\n", client->node.socket.handle);
                success = false;
            }
            
            if(!success) {
                platform_close_socket(&client->node.socket);
                clear(client);
                continue;
            }
                 
            client = array_add(server->clients, *client);
            MS_Log("Added client (socket = %lld) to market server.\n", client->node.socket.handle);

            if(client->type == MS_CLIENT_PLAYER)
            {
                MCB_Packet(MARKET_INIT, client);
                if(!client) {
                    MS_Log("Unable to init market client.\n"); // initialize_user_client() disconnects client for us on error.
                }
            }
        }

        queue->num_clients = 0;
    }
    unlock_mutex(queue->mutex);
}


bool read_and_handle_msb_packet(MS_Client *client, MSB_Packet_Header header)
{
    auto *node = &client->node;
    
    switch(header.type) {
        case MSB_PLACE_ORDER: {
            auto &p = header.place_order;

            const char *sell_or_buy = (p.is_buy_order) ? "buy" : "sell";
            String item_type_name = item_types[p.item_type].name;
            MS_Log("User %lld wants to %s a %.*s for ¤%lld.\n", client->user_id, sell_or_buy, (int)item_type_name.length, item_type_name.data, p.price);
            
        } break;

        default: {
            MS_Log("Client (socket = %lld) sent invalid MSB packet type (%u).\n", client->node.socket.handle, header.type);
            return false;
        } break;
    }

    return true;
}

// REMEMBER to init_market_server before starting this.
//          IMPORTANT: Do NOT deinit_market_server after
//                     this is done -- this proc will do that for you.
DWORD market_server_main_loop(void *server_) {

    const Allocator_ID allocator = ALLOC_MS;
    
    Market_Server *server = (Market_Server *)server_;
    defer(deinit_market_server(server););
    MS_Log("Running.\n");

    auto *listening_loop = &server->listening_loop;

    // INITIALIZE MODEL //
    // TODO

    // START LISTENING LOOP //
    Thread listening_loop_thread;
    if(!start_market_server_listening_loop(server, &listening_loop_thread))
    {
        MS_Log("Failed to start listening loop.");
        return 1; // TODO @Norelease: Main server program must know about this!
    }
    
    while(!get(&server->should_exit)) {

        if(get(&listening_loop->client_accept_failed)) {
            // RESTART LISTENING LOOP //
            MS_Log("Listening loop failed. Joining thread...\n");
            platform_join_thread(listening_loop_thread);
            
            MS_Log("Restarting listening loop...\n");
            deinit_listening_loop(listening_loop);
            if(!start_market_server_listening_loop(server, &listening_loop_thread)) {
                MS_Log("Failed to restart listening loop.\n");
                // TODO @Norelease: Disconnect all clients. (Say goodbye etc)
                // TODO @Norelease: Main server program must know about this!
                return 1;
            }
        }


        // LISTEN TO WHAT THE CLIENTS HAVE TO SAY //
        for(int c = 0; c < server->clients.n; c++) {
            auto *client = &server->clients[c];

            while(true) {
                    
                bool error;
                if(!receive_next_network_node_packet(&client->node, &error)) {
                    if(error) {
                        disconnect_market_client(client); // @Cleanup @Boilerplate
                        c--;
                    }
                    break;
                }
                    
                MSB_Packet_Header header;
                MSB_Header(client, &header);
                if(client == NULL) {
                    c--;
                    break;
                }

                bool do_disconnect = false;
                    
                if(header.type == MSB_GOODBYE) {
                    // TODO @Norelease: Better logging, with more client info etc.
                    MS_Log("Client (socket = %lld) sent goodbye message.\n", client->node.socket.handle);
                    do_disconnect = true;
                }
                else if(!read_and_handle_msb_packet(client, header)) {
                    do_disconnect = true;
                }

                if (do_disconnect) { // @Cleanup @Boilerplate
                    disconnect_market_client(client);
                    c--;
                    break;
                }
                    
            }
        }
        
        // SEND UPDATES TO PLAYER CLIENTS //
        for(int c = 0; c < server->clients.n; c++) {
            auto *client = &server->clients[c];
            if(client->type != MS_CLIENT_PLAYER) continue;

            if(false /* @Norelease */) {
                MCB_Packet(MARKET_UPDATE, client);
            }
        }

        add_new_market_clients(server);
        platform_sleep_milliseconds(1);
    }

    MS_Log("Stopping listening loop...");
    stop_listening_loop(listening_loop, &listening_loop_thread);
    MS_Log_No_T("Done.\n");
    
    MS_Log("Exiting.\n");

    return 0;
}
