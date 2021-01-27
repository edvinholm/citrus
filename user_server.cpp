
#include "user_server_bound.cpp"
#include "user_client_bound.cpp"


const char *LOG_TAG_US = "-US-"; // User Server
const char *LOG_TAG_US_LIST = "-US-LIST-"; // Listening loop

#define US_Log_T(Tag, ...) \
    printf("[%s]", LOG_TAG_US);                            \
    Log_T(Tag, __VA_ARGS__)
#define US_Log(...)                     \
    Log_T(LOG_TAG_US, __VA_ARGS__)
#define US_Log_No_T(...)                     \
    Log(__VA_ARGS__)


#define US_LIST_Log_T(Tag, ...) \
    printf("[%s]", LOG_TAG_US_LIST);      \
    Log_T(Tag, __VA_ARGS__)
#define US_LIST_Log(...)                     \
    Log_T(LOG_TAG_US_LIST, __VA_ARGS__)
#define US_LIST_No_T(...)                     \
    Log(__VA_ARGS__)



void create_dummy_users(User_Server *server, Allocator_ID allocator)
{
    char *usernames[] = {
        "Tachophobia",
        "Sailor88",
        "WhoLetTheDogsOut",
        "MrCool",
        "kadlfgAJb!",
        "LongLongWay.9000",
        "_u_s_e_r_n_a_m_e_",
        "generalW4ste"
    };
    
    Array<US_Client, ALLOC_APP> empty_client_array = {0};

    User_ID next_user_id = 1;
    Item_ID next_item_id = 1;
    
    for(int i = 0; i < ARRLEN(usernames); i++) {
        User user = {0};
        user.shared.id = next_user_id++; 
        user.shared.username = copy_cstring_to_string(usernames[i], allocator);
        user.shared.color = { random_float(),  random_float(),  random_float(), 1 };

        Assert(user.shared.id != NO_USER);

        for(int j = 0; j < ARRLEN(user.shared.inventory); j++)
        {
            Item item = {0};
            item.id   = next_item_id++;
            item.type = (Item_Type_ID)random_int(0, ITEM_NONE_OR_NUM);
            user.shared.inventory[j] = item;
        }
        
        array_add(server->users,   user);
        array_add(server->clients, empty_client_array);
    }

    US_Log("Dummy users:\n");
    for(int i = 0; i < server->users.n; i++) {
        auto &sh = server->users[i].shared;
        auto &c = sh.color;

        int num_spaces = max(1, 32 - sh.username.length);
        US_Log("    %.*s", (int)sh.username.length, sh.username.data);
        for(int i = 0; i < num_spaces; i++) {
            US_Log_No_T(" ");
        }
        US_Log_No_T("{%.2f, %.2f, %.2f, %.2f}\n", c.r, c.g, c.b, c.a);
    }
}

void enqueue_client(US_Client *client, US_Client_Queue *queue)
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



bool receive_next_usb_packet(Network_Node *node, USB_Packet_Header *_packet_header, bool *_error, bool block = false)
{
    if(!receive_next_network_node_packet(node, _error, block)) return false;
    
    *_error = true;
    Read_To_Ptr(USB_Packet_Header, _packet_header, node);
    *_error = false;
    
    return true;
}

bool expect_type_of_next_usb_packet(USB_Packet_Type expected_type, Network_Node *node, USB_Packet_Header *_packet_header)
{
    bool error;
    if(!receive_next_usb_packet(node, _packet_header, &error, true)) return false;
    if(_packet_header->type != expected_type) return false;
    return true;
}

// TODO @Norelease @Robustness @Speed: This thing should have enough information to dismiss clients with invalid user IDs or auth tokens.
DWORD user_server_listening_loop(void *user_server_)
{
    auto *server = (User_Server *)user_server_;
    auto *queue  = &server->client_queue;

    Listening_Loop *loop = &server->listening_loop;

    US_LIST_Log("Running.\n");
    
    bool   client_accepted;
    Socket client_socket;
    while(listening_loop_running(loop, &client_accepted, &client_socket, LOG_TAG_US_LIST))
    {
        if(!client_accepted) continue;

        US_LIST_Log("Client accepted :)\n");

        bool success = true;
        
        if(success && !platform_set_socket_read_timeout(&client_socket, 1000)) {
            US_LIST_Log("Unable to set read timeout for new client's socket (Last WSA Error: %d).\n", WSAGetLastError());
            success = false;
        }

        Network_Node new_node = {0};
        reset_network_node(&new_node, client_socket);

        USB_Packet_Header header;
        if(success && !expect_type_of_next_usb_packet(USB_HELLO, &new_node, &header)) {
            US_LIST_Log("First packet did not have the expected type USB_HELLO.\n");
            success = false;
        }

        if(!success)
        {
           if(!platform_close_socket(&client_socket)) {
               US_LIST_Log("Unable to close new client's socket.\n");
           } else {
               US_LIST_Log("New client's socket closed successfully.\n");
           }
           continue;
        }

        auto *hello = &header.hello;
        
        // ADD CLIENT TO QUEUE //
        US_Client client = {0};
        client.type     = hello->client_type;
        client.user_id  = hello->user;
        client.server   = server;
        client.node     = new_node;

        US_LIST_Log("Requested user: %llu\n", client.user_id);

        enqueue_client(&client, queue);

        platform_sleep_milliseconds(1);
    }

    US_LIST_Log("Exiting.");
    return 0;
}

void commit_transaction(USB_Transaction t, User *user)
{
    switch(t.type) {
        case USB_T_ITEM: {

            for(int i = 0; i < ARRLEN(user->shared.inventory); i++) {
                if(user->shared.inventory[i].id == t.item_details.item.id) {
                    // @Boilerplate: Client: world.cpp: empty_inventory_slot_locally()
                    Zero(user->shared.inventory[i]);
                    user->shared.inventory[i].id = NO_ITEM;
                    user->shared.inventory[i].type = ITEM_NONE_OR_NUM;
                    user->inventory_changed = true;
                    return;
                }
            }
            
            Assert(false);
        } break;

        default: Assert(false); return;
    }
}

bool transaction_possible(USB_Transaction t, User *user)
{
    switch(t.type) {
        case USB_T_ITEM: {
            for(int i = 0; i < ARRLEN(user->shared.inventory); i++) {
                if(equal(&user->shared.inventory[i], &t.item_details.item))
                    return true;
            }
            return false;
        } break;

        default: Assert(false); return false;
    }
}

bool read_and_handle_usb_packet(US_Client *client, USB_Packet_Header header, User *user)
{
    // NOTE: USB_GOODBYE is handled somewhere else.

    auto *node = &client->node;
    
    switch(header.type) {

        case USB_TRANSACTION_MESSAGE: {
            auto &p = header.transaction_message;
            
            switch(p.message) {

                case TRANSACTION_PREPARE: {
                    if(transaction_possible(p.transaction, user)) {
                        Send_Now(UCB_TRANSACTION_MESSAGE, &client->node, TRANSACTION_VOTE_COMMIT);
                        commit_transaction(p.transaction, user);
                    } else {
                        Send_Now(UCB_TRANSACTION_MESSAGE, &client->node, TRANSACTION_VOTE_ABORT);

                        // IMPORTANT: If we fail to do the transaction, we still want to send a USER_UPDATE.
                        //            This is so the game client knows when to for example start showing the inventory item
                        //            again that the player was trying to place.
                        // (@Hack)
                        user->inventory_changed = true;
                    }
                } break;

                default: Assert(false); return false;
            }
        } break;
        
        default: {
            US_Log("Client (socket = %lld) sent invalid USB packet type (%u).\n", client->node.socket.handle, header.type);
            return false;
        } break;
    }
    
    return true;
}


inline
bool start_user_server_listening_loop(User_Server *server, Thread *_thread)
{
    return start_listening_loop(&server->listening_loop, USER_SERVER_PORT, &user_server_listening_loop, server, _thread, LOG_TAG_US_LIST);
}


void disconnect_user_client(US_Client *client)
{
    auto client_copy = *client;
    
    bool found_user = false;
    
    auto *server = client->server;
    for(int i = 0; i < server->users.n; i++)
    {
        if(server->users[i].shared.id == client->user_id) {
            auto *clients = &server->clients[i];
            Assert(client >= clients->e);
            Assert(client < clients->e + clients->n);

            auto client_index = client - clients->e;
            array_ordered_remove(*clients, client_index);
            
            found_user = true;
            break;
        }
    }
    Assert(found_user);

    // REMEMBER: This is special, so don't do anything fancy here that can put us in an infinite disconnection loop.
    if(!send_UCB_GOODBYE_packet_now(&client->node)) {
        US_Log("Failed to write UCB Goodbye.\n");
    }
    
    if(!platform_close_socket(&client->node.socket)) {
        US_Log("Failed to close user client socket.\n");
    }

    // IMPORTANT that we clear the copy here. Otherwise we would clear the client that is placed where our client was in the array.
    clear(&client_copy);
}



// Sends an UCB packet if US_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// US_Client_Ptr is set to NULL.
#define UCB_Packet(Packet_Ident, US_Client_Ptr, ...)                    \
    if(US_Client_Ptr) {                                                 \
        bool success = true;                                            \
        if(success && !enqueue_UCB_##Packet_Ident##_packet(&US_Client_Ptr->node, __VA_ARGS__)) success = false; \
        Assert(US_Client_Ptr->node.packet_queue.n == 1);                \
        if(success && !send_outbound_packets(&US_Client_Ptr->node)) success = false; \
        if(!success) {                                                  \
            US_Log("Client disconnected from user %lld due to UCB packet failure (socket = %lld, WSA Error: %d).\n", US_Client_Ptr->user_id, US_Client_Ptr->node.socket.handle, WSAGetLastError()); \
            disconnect_user_client(US_Client_Ptr);                      \
            US_Client_Ptr = NULL;                                       \
        }                                                               \
    }


// Receives an USB packet if US_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// US_Client_Ptr is set to NULL.
#define USB_Header(US_Client_Ptr, Header_Ptr)                       \
    if((US_Client_Ptr) &&                                             \
       !read_USB_Packet_Header(Header_Ptr, &US_Client_Ptr->node)) {    \
        US_Log("Client disconnected from user (id = %u) due to USB header failure (socket = %lld, WSA Error: %d).\n", (int)US_Client_Ptr->user_id, US_Client_Ptr->node.socket.handle, WSAGetLastError()); \
        disconnect_user_client(US_Client_Ptr);                        \
        US_Client_Ptr = NULL;                                         \
    }                                                               \




void add_new_user_clients(User_Server *server)
{
    auto *queue = &server->client_queue;
    lock_mutex(queue->mutex);
    {
        Assert(queue->num_clients <= ARRLEN(queue->clients));
        
        for(int c = 0; c < queue->num_clients; c++) {

            US_Client *client = queue->clients + c;
            User_ID requested_user = client->user_id;

            User *user = NULL;
            int user_index = -1;
            for(int u = 0; u < server->users.n; u++) {
                if(server->users[u].shared.id == requested_user) {
                    user_index = u;
                    user = server->users.e + u;
                    break;
                }
            }

            bool success = true;

            if(success && user_index == -1) {
                if(!send_UCB_HELLO_packet_now(&client->node, USER_CONNECT__INCORRECT_CREDENTIALS)) {
                    US_Log("Failed to write USER_CONNECT__INCORRECT_CREDENTIALS to client (socket = %lld).\n", client->node.socket.handle);
                }
                success = false;
            }

            if(success && !send_UCB_HELLO_packet_now(&client->node, USER_CONNECT__CONNECTED))
            {
                US_Log("Failed to write USER_CONNECT__CONNECTED to client (socket = %lld).\n", client->node.socket.handle);
                success = false;
            }
            
            if(!success) {
                platform_close_socket(&client->node.socket);
                clear(client);
                continue;
            }
            
            Assert(user != NULL);
                 
            auto *clients = &server->clients[user_index];
            client = array_add(*clients, *client);
            US_Log("Added client (socket = %lld) to user (username = %.*s).\n", client->node.socket.handle, (int)user->shared.username.length, user->shared.username.data);

            if(client->type == US_CLIENT_PLAYER)
            {
                auto &u = user->shared;
                UCB_Packet(USER_INIT, client, u.id, u.username, u.color, u.inventory);
                if(!client) {
                    US_Log("Unable to init user client.\n"); // initialize_user_client() disconnects client for us on error.
                }
            }
        }

        queue->num_clients = 0;
    }
    unlock_mutex(queue->mutex);
}


// REMEMBER to init_user_server before starting this.
//          IMPORTANT: Do NOT deinit_user_server after
//                     this is done -- this proc will do that for you.
DWORD user_server_main_loop(void *server_) {

    const Allocator_ID allocator = ALLOC_US;
    
    User_Server *server = (User_Server *)server_;
    defer(deinit_user_server(server););
    US_Log("Running.\n");

    auto *listening_loop = &server->listening_loop;

    // INITIALIZE MODEL //
    create_dummy_users(server, allocator);

    // START LISTENING LOOP //
    Thread listening_loop_thread;
    if(!start_user_server_listening_loop(server, &listening_loop_thread))
    {
        US_Log("Failed to start listening loop.");
        return 1; // TODO @Norelease: Main server program must know about this!
    }
    
    while(!get(&server->should_exit)) {

        if(get(&listening_loop->client_accept_failed)) {
            // RESTART LISTENING LOOP //
            US_Log("Listening loop failed. Joining thread...\n");
            platform_join_thread(listening_loop_thread);
            
            US_Log("Restarting listening loop...\n");
            deinit_listening_loop(listening_loop);
            if(!start_user_server_listening_loop(server, &listening_loop_thread)) {
                US_Log("Failed to restart listening loop.\n");
                // TODO @Norelease: Disconnect all clients. (Say goodbye etc)
                // TODO @Norelease: Main server program must know about this!
                return 1;
            }
        }


        Assert(server->clients.n == server->users.n);
        for(int i = 0; i < server->users.n; i++)
        {
            User *user    = &server->users[i];
            S__User *s_user = &user->shared;
            auto &clients = server->clients[i];
            
            // LISTEN TO WHAT THE CLIENTS HAVE TO SAY //
            for(int c = 0; c < clients.n; c++) {
                auto *client = &clients[c];

                while(true) {

                    bool error;
                    if(!receive_next_network_node_packet(&client->node, &error)) {
                        if(error) {
                            disconnect_user_client(client); // @Cleanup @Boilerplate
                            c--;
                        }
                        break;
                    }
                    
                    USB_Packet_Header header;
                    USB_Header(client, &header);
                    if(client == NULL) { c--; break; }

                    bool do_disconnect = false;

                    if(header.type == USB_GOODBYE) {
                        // TODO @Norelease: Better logging, with more client info etc.
                        RS_Log("Client (socket = %lld) sent goodbye message.\n", client->node.socket.handle);
                        do_disconnect = true;
                    }
                    else if(!read_and_handle_usb_packet(client, header, user)) {
                        do_disconnect = true;
                    }
                    
                    if (do_disconnect) { // @Cleanup @Boilerplate
                        disconnect_user_client(client);
                        c--;
                        break;
                    }
                }
            }

            // SEND UPDATES TO PLAYER CLIENTS //
            for(int c = 0; c < clients.n; c++) {
                auto *client = &clients[c];
                if(client->type != US_CLIENT_PLAYER) continue;

                if(user->inventory_changed) {
                    UCB_Packet(USER_UPDATE, client, s_user->id, s_user->username, s_user->color, s_user->inventory);
                }
            }

            user->inventory_changed = false;
        }
        

        add_new_user_clients(server);
        platform_sleep_milliseconds(100);
    }

    US_Log("Stopping listening loop...");
    stop_listening_loop(listening_loop, &listening_loop_thread);
    US_Log_No_T("Done.\n");
    
    US_Log("Exiting.\n");

    return 0;
}
