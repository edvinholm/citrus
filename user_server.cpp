
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
    
    Array<User_Client, ALLOC_APP> empty_client_array = {0};
    
    for(int i = 0; i < ARRLEN(usernames); i++) {
        User user = {0};
        user.shared.username = copy_cstring_to_string(usernames[i], allocator);
        user.shared.color = { random_float(),  random_float(),  random_float(), 1 };
        
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

void enqueue_client(User_Client *client, User_Client_Queue *queue)
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

// TODO @Norelease @Robustness @Speed: This thing should have enough information to dismiss clients with invalid login credentials.
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

        String requested_username;
        if(success && !read_String(&requested_username, &client_socket, ALLOC_US)) {
            US_LIST_Log("Unable to read requested username from new client.\n");
            success = false;
        }

        if(success)
        {
            // ADD CLIENT TO QUEUE //
            User_Client client = {0};
            client.sock     = client_socket;
            client.username = requested_username;
            client.server   = server;

            US_LIST_Log("Requested user: \"%.*s\"\n", (int)client.username.length, client.username.data);

            enqueue_client(&client, queue);
        }
        else
        {
           if(!platform_close_socket(&client_socket)) {
               US_LIST_Log("Unable to close new client's socket.\n");
           } else {
               US_LIST_Log("New client's socket closed successfully.\n");
           }
           continue;
        }
    }

    US_LIST_Log("Exiting.");
    return 0;
}

bool read_and_handle_usb_packet(User_Client *client, USB_Packet_Header header, User *user)
{
    // NOTE: USB_GOODBYE is handled somewhere else.

    auto *sock = &client->sock;
    
    switch(header.type) {

        
        
        default: {
            US_Log("Client (socket = %lld) sent invalid USB packet type (%u).\n", client->sock.handle, header.type);
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


void disconnect_user_client(User_Client *client)
{
    auto client_copy = *client;
    
    bool found_user = false;
    
    auto *server = client->server;
    for(int i = 0; i < server->users.n; i++)
    {
        if(equal(server->users[i].shared.username, client->username)) {
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
    if(!write_ucb_Goodbye_packet(&client->sock)) {
        US_Log("Failed to write UCB Goodbye.\n");
    }
    
    if(!platform_close_socket(&client->sock)) {
        US_Log("Failed to close user client socket.\n");
    }

    // IMPORTANT that we clear the copy here. Otherwise we would clear the client that is placed where our client was in the array.
    clear(&client_copy);
}



// Sends an UCB packet if User_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// User_Client_Ptr is set to NULL.
#define UCB_Packet(User_Client_Ptr, Packet_Ident, ...)                  \
    if(User_Client_Ptr &&                                               \
       !write_ucb_##Packet_Ident##_packet(&User_Client_Ptr->sock, __VA_ARGS__)) \
    {                                                                   \
        US_Log("Client disconnected from user (username = %.*s) due to UCB packet failure (socket = %lld, WSA Error: %d).\n", (int)User_Client_Ptr->username.length, User_Client_Ptr->username.data, User_Client_Ptr->sock.handle, WSAGetLastError()); \
        disconnect_user_client(User_Client_Ptr);                      \
        User_Client_Ptr = NULL;                                         \
    }


// Receives an USB packet if User_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// User_Client_Ptr is set to NULL.
#define USB_Header(User_Client_Ptr, Header_Ptr)                       \
    if((User_Client_Ptr) &&                                             \
       !read_USB_Packet_Header(Header_Ptr, &User_Client_Ptr->sock)) {    \
        US_Log("Client disconnected from user (username = %.*s) due to USB header failure (socket = %lld, WSA Error: %d).\n", (int)User_Client_Ptr->username.length, User_Client_Ptr->username.data, User_Client_Ptr->sock.handle, WSAGetLastError()); \
        disconnect_user_client(User_Client_Ptr);                        \
        User_Client_Ptr = NULL;                                         \
    }                                                               \




bool initialize_user_client(User_Client *client, User *user)
{
    UCB_Packet(client, User_Init, user->shared.username, user->shared.color);
    return true;
}


void add_new_user_clients(User_Server *server)
{
    auto *queue = &server->client_queue;
    lock_mutex(queue->mutex);
    {
        Assert(queue->num_clients <= ARRLEN(queue->clients));
        
        for(int c = 0; c < queue->num_clients; c++) {

            User_Client *client = queue->clients + c;
            String requested_username = client->username;

            User *user = NULL;
            int user_index = -1;
            for(int u = 0; u < server->users.n; u++) {
                if(equal(server->users[u].shared.username, requested_username)) { // @Speed
                    user_index = u;
                    user = server->users.e + u;
                    break;
                }
            }

            bool success = true;
            
            if(success && user_index == -1) {
                write_user_connect_status_code(USER_CONNECT__INCORRECT_CREDENTIALS, &client->sock);
                success = false;
            }

            if(success && !write_user_connect_status_code(USER_CONNECT__CONNECTED, &client->sock))
            {
                RS_Log("Failed to write USER_CONNECT__CONNECTED to client (socket = %lld).\n", client->sock.handle);
                success = false;
            }
            
            if(!success) {
                platform_close_socket(&client->sock);
                clear(client);
                continue;
            }
            
            Assert(user != NULL);
                 
            auto *clients = &server->clients[user_index];
            client = array_add(*clients, *client);
            RS_Log("Added client (socket = %lld) to user (username = %.*s).\n", client->sock.handle, (int)user->shared.username.length, user->shared.username.data);

            if(!initialize_user_client(client, user)) {
                US_Log("Unable to init user client.\n"); // initialize_user_client() disconnects client for us on error.
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
                // TODO @Norelease Disconnect all clients. (Say goodbye etc)
                // TODO @Norelease: Main server program must know about this!
                return 1;
            }
        }


        Assert(server->clients.n == server->users.n);
        for(int i = 0; i < server->users.n; i++)
        {
            User *user    = &server->users[i];
            auto &clients = server->clients[i];
            
            // LISTEN TO WHAT THE CLIENTS HAVE TO SAY //
            for(int c = 0; c < clients.n; c++) {
                auto *client = &clients[c];

                while(true) {

                    bool do_disconnect = false;
                    
                    bool error;
                    if(!platform_socket_has_bytes_to_read(&client->sock, &error)) { // TODO @Norelease: @Security: There has to be some limit to how much data a client can send, so that we can't be stuck in this loop forever!
                        if(error) { do_disconnect = true; }
                        else break;
                    }

                    if(!do_disconnect) {
                        USB_Packet_Header header;
                        USB_Header(client, &header);
                        if(client == NULL) { c--; break; }

                        if(header.type == USB_GOODBYE) {
                            // TODO @Norelease: Better logging, with more client info etc.
                            US_Log("Client (socket = %lld) sent goodbye message.\n", client->sock.handle);
                            do_disconnect = true;
                        }
                        else if(!read_and_handle_usb_packet(client, header, user)) {
                            do_disconnect = true;
                        }
                    }
                    
                    if (do_disconnect) { // @Cleanup @Boilerplate
                        disconnect_user_client(client);
                        c--;
                        break;
                    }
                }
            }
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
