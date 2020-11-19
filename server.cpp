
double get_time() {
    return (double)platform_performance_counter() / (double)platform_performance_counter_frequency();
}


void create_dummy_rooms(Server *server)
{
    double t = get_time();

    Array<Room_Client, ALLOC_APP> empty_client_array = {0};
    
    for(int i = 0; i < 8; i++) {
        Room room = {0};
        room.t = t;
        room.randomize_cooldown = random_float() * 3.0;
        
        array_add(server->room_ids, i + 1);
        array_add(server->rooms,    room);
        
        array_add(server->room_clients, empty_client_array);
    }
}

inline
Tile random_tile(int x)
{
    Tile t = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
    switch(x) {
        case 2: if(t == TILE_GRASS) t = TILE_WATER; break;
        case 4: if(t == TILE_STONE) t = TILE_GRASS; break;
        case 6: if(t == TILE_GRASS || t == TILE_STONE) t = TILE_WATER; break;
        case 8: if(t == TILE_WATER || t == TILE_SAND)  t = TILE_GRASS; break;
        default: break;
    }

    return t;
}

void randomize_tiles(Tile *tiles, u64 num_tiles, int x) {
    Tile *at = tiles;
    Tile *end = tiles + num_tiles;

    while(end-at >= 8) {
        at[0] = random_tile(x);
        at[1] = random_tile(x);
        at[2] = random_tile(x);
        at[3] = random_tile(x);
        at[4] = random_tile(x);
        at[5] = random_tile(x);
        at[6] = random_tile(x);
        at[7] = random_tile(x);
        at += 8;
    }

    while(at < end) {
        *at = random_tile(x);
        at++;
    }
}

void update_room(Room *room, int index) {

    Assert(room->t > 0); // Should be initialized when created
    
    double last_t = room->t;
    room->t = get_time();
    double dt = room->t - last_t;

    room->randomize_cooldown -= dt;
    while(room->randomize_cooldown <= 0) {
        Tile *tiles = room->shared.tiles;
        Tile *at  = tiles;
        Tile *end = tiles + room_size_x * room_size_y;
        while(at < end) {
            
            if(*at == TILE_WATER) {
                
                int y = (at - tiles) / room_size_x;
                int x = (at - tiles) % room_size_x;
                
                if(y > 0) {
                    
                    Tile *north = at - room_size_x;
                    
                    if(*north != TILE_WATER) {
                        if(*north != TILE_STONE) {
                            *north = TILE_WATER;
                        }
                        *at = TILE_GRASS;
                        room->did_change = true;
                    }
                }
            }
            
            at++;
        }
#if 0
        randomize_tiles(room->shared.tiles, ARRLEN(room->shared.tiles), random_int(1, 10));
        room->did_change = true;
#endif
        room->randomize_cooldown += 0.2;
    }
}

bool setup_listening_socket(Socket *_sock)
{
    if(!platform_create_tcp_socket(_sock)) return false;
    
    if(!platform_bind_socket(_sock, SERVER_PORT)) return false;
    if(!platform_start_listening_to_socket(_sock, LISTENING_SOCKET_BACKLOG_SIZE)) return false;
    
    return true;
}


// TODO @Norelease @Robustness @Speed: This thing should have enough information to dismiss clients with invalid room IDs or login credentials.
DWORD listening_loop(void *server_)
{    
    Server *server = (Server *)server_;
    Socket listening_socket = server->listening_socket;
    auto *queue = &server->room_client_queue;
    
    while(true) {
        Socket client_socket;

        // @Norelease: Get IP address.
        bool error;
        if(!platform_accept_next_incoming_socket_connection(&listening_socket, &client_socket, &error)) {
            if(!error) continue;

            Debug_Print("Listening loop client accept failure\n");
            platform_close_socket(&listening_socket);
            set(&server->listening_loop_client_accept_failed, true);

            break;
        }
        
        Debug_Print("Client accepted.\n");

        // @Cleanup close socket boilerplate...
        // @Cleanup close socket boilerplate...
        // @Cleanup close socket boilerplate...

        if(!platform_set_socket_read_timeout(&client_socket, 1000)) {
            Debug_Print("Unable to set read timeout for new client's socket.\n");
            
            if(!platform_close_socket(&client_socket)) {
                Debug_Print("Unable to close new client's socket.\n");
                continue;
            }
            Debug_Print("New client's socket closed successfully.\n");
            continue;
        }

        u64 requested_room_id;
        if(!read_u64(&requested_room_id, &client_socket)) {
            Debug_Print("Unable to read requested room ID from new client.\n");

            if(!platform_close_socket(&client_socket)) {
                Debug_Print("Unable to close new client's socket.\n");
                continue;
            }
            Debug_Print("New client's socket closed successfully.\n");
            continue;
        }

        if(!write_room_connect_status_code(ROOM_CONNECT__REQUEST_RECEIVED, &client_socket)) {
            Debug_Print("Unable to write status.\n");
        }

        // ADD CLIENT TO QUEUE //
        Room_Client client = {0};
        client.sock = client_socket;
        client.room = requested_room_id;
        client.server = server;

        lock_mutex(queue->mutex);
        {
            Assert(queue->num_clients <= ARRLEN(queue->clients));
            Assert(ARRLEN(queue->clients) <= ARRLEN(queue->rooms));

            // If the queue is full, wait..
            while(queue->num_clients == ARRLEN(queue->clients)) {
                unlock_mutex(queue->mutex);
                platform_sleep_microseconds(100);
                lock_mutex(queue->mutex);
            }
            
            Assert(queue->num_clients <= ARRLEN(queue->clients));
            Assert(ARRLEN(queue->clients) <= ARRLEN(queue->rooms));
                
            int ix = queue->num_clients++;
            queue->clients[ix] = client;
            queue->rooms[ix] = requested_room_id;
        }
        unlock_mutex(queue->mutex);
        // /// ///// // ///// //
    }

    Debug_Print("Listening loop exiting.\n");
    return 0;
}

void disconnect_room_client(Room_Client *client)
{
    auto client_copy = *client;
    
    bool found_room = false;
    
    auto *server = client->server;
    Assert(server->room_ids.n == server->room_clients.n);
    for(int i = 0; i < server->room_ids.n; i++)
    {
        if(server->room_ids[i] == client->room) {
            auto *clients = &server->room_clients[i];
            Assert(client >= clients->e);
            Assert(client < clients->e + clients->n);

            auto client_index = client - clients->e;
            array_ordered_remove(*clients, client_index);
            
            found_room = true;
            break;
        }
    }
    Assert(found_room);
    
    // REMEMBER: This is special, so don't do anything fancy here that can put us in an infinite disconnection loop.
    if(!write_rcb_Goodbye_packet(&client->sock)) {
        Debug_Print("Failed to write RCB Goodbye.\n");
    }
    
    if(!platform_close_socket(&client->sock)) {
        Debug_Print("Failed to close room client socket.\n");
    }

    // IMPORTANT that we clear the copy here. Otherwise we would clear the client that is placed where our client was in the array.
    clear(&client_copy);
}


// Sends an RCB packet if Room_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// Room_Client_Ptr is set to NULL.
#define RCB_Packet(Room_Client_Ptr, Packet_Ident, ...)                  \
    if(Room_Client_Ptr &&                                               \
       !write_rcb_##Packet_Ident##_packet(&Room_Client_Ptr->sock, __VA_ARGS__)) \
    {                                                                   \
        Debug_Print("Client disconnected from room %d due to RCB packet failure (socket = %lld, WSA Error: %d).\n", Room_Client_Ptr->room, Room_Client_Ptr->sock.handle, WSAGetLastError()); \
        disconnect_room_client(Room_Client_Ptr);                      \
        Room_Client_Ptr = NULL;                                         \
    }


// Receives an RSB packet if Room_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// Room_Client_Ptr is set to NULL.
#define RSB_Header(Room_Client_Ptr, Header_Ptr)                       \
    if((Room_Client_Ptr) &&                                             \
       !read_RSB_Packet_Header(Header_Ptr, &Room_Client_Ptr->sock)) {    \
        Debug_Print("Client disconnected from room %d due to RSB header failure (socket = %lld, WSA Error: %d).\n", Room_Client_Ptr->room, Room_Client_Ptr->sock.handle, WSAGetLastError()); \
        disconnect_room_client(Room_Client_Ptr);                        \
        Room_Client_Ptr = NULL;                                         \
    }                                                               \



bool initialize_room_client(Room_Client *client, Room *room)
{
    RCB_Packet(client, Room_Init, room->shared.tiles);
    return true;
}

void add_new_room_clients(Server *server)
{
    auto *queue = &server->room_client_queue;
    lock_mutex(queue->mutex);
    {
        Assert(queue->num_clients <= ARRLEN(queue->clients));
        Assert(ARRLEN(queue->clients) == ARRLEN(queue->rooms));
        for(int c = 0; c < queue->num_clients; c++) {

            Room_ID room_id = queue->rooms[c];

            Room *room = NULL;
            
            int room_index = -1;
            for(int r = 0; r < server->room_ids.n; r++) {
                if(server->room_ids[r] == room_id) {
                    room_index = r;
                    room = server->rooms.e + r;
                    break;
                }
            }

            Room_Client *client = queue->clients + c;
                
            if(room_index == -1) {
                write_room_connect_status_code(ROOM_CONNECT__INVALID_ROOM_ID, &client->sock);

                // @Cleanup @Boilerplate                
                // IMPORTANT: Do not use disconnect_room_client here, because it won't work before we've added the client to the room client list.
                platform_close_socket(&client->sock);
                clear(client);
                
                continue;
            }

            if(!write_room_connect_status_code(ROOM_CONNECT__CONNECTED, &client->sock))
            {
                Debug_Print("Failed to write ROOM_CONNECT__CONNECTED to client (socket = %lld).\n", client->sock.handle);

                // @Cleanup @Boilerplate
                platform_close_socket(&client->sock);
                clear(client);
                continue;
            }
                        
            Assert(room != NULL);
            if(!initialize_room_client(client, room)) {
                // @Cleanup @Boilerplate
                platform_close_socket(&client->sock);
                clear(client);
                continue;
            }

            auto *clients = &server->room_clients[room_index];
            array_add(*clients, *client);
            Debug_Print("Added client (socket = %lld) to room %d.\n", client->sock.handle, client->room);
        }

        queue->num_clients = 0;
    }
    unlock_mutex(queue->mutex);
}

// NOTE: Assumes we've already zeroed queue.
void init_room_client_queue(Room_Client_Queue *queue)
{
    create_mutex(queue->mutex);
}

bool start_listening_loop(Server *server, Thread *thread)
{
    while(!setup_listening_socket(&server->listening_socket))
    {
        Debug_Print("setup_listening_socket() failed. Retrying in 3...");
        platform_sleep_microseconds(1000*1000);
        Debug_Print("2..."); platform_sleep_microseconds(1000*1000);
        Debug_Print("1..."); platform_sleep_microseconds(1000*1000);
        Debug_Print("\n");
    }
    if(!platform_create_thread(&listening_loop, server, thread)) {
        Debug_Print("Unable to create listening loop thread.\n");
        false;
    }
    return true;
}

bool read_and_handle_rsb_packet(Room_Client *client, RSB_Packet_Header header, Room *room)
{
    // NOTE: RSB_GOODBYE is handled somewhere else.

    auto *sock = &client->sock;
    
    switch(header.type) {
        case RSB_CLICK_TILE: {
            Read(u64, tile_ix, sock);

            Fail_If_True(tile_ix >= room_size_x * room_size_y);
            room->shared.tiles[tile_ix]++;
            room->shared.tiles[tile_ix] %= TILE_NONE_OR_NUM;
            room->did_change = true;
            
        } break;
            
        default: {
            Debug_Print("Client (socket = %lld) sent invalid RSB packet type (%u).\n", client->sock.handle, header.type);
            return false;
        } break;
    }
    
    return true;
}

int server_entry_point(int num_args, char **arguments)
{
    Debug_Print("I am a server.\n");

    Server server = {0};

    init_atomic(&server.listening_loop_client_accept_failed);
    defer(deinit_atomic(&server.listening_loop_client_accept_failed););
    
    init_room_client_queue(&server.room_client_queue);

    
    create_dummy_rooms(&server);

    
    if(!platform_init_socket_use()) { Debug_Print("platform_init_socket_use() failed.\n"); return 1; }

    // START LISTENING LOOP //
    Thread listening_loop_thread;
    if(!start_listening_loop(&server, &listening_loop_thread))
        return 3;
    // 
           
    double t = get_time();


    Array<int, ALLOC_APP> clients_to_disconnect = {0};
    while(true) {

        if(get(&server.listening_loop_client_accept_failed))
        {
            Debug_Print("Listening loop failed. Joining thread...\n");
            platform_join_thread(listening_loop_thread);
            Debug_Print("Restarting listening loop...\n");
            if(!start_listening_loop(&server, &listening_loop_thread)) {
                // TODO @Norelease Disconnect all clients. (Say goodbye etc)
                return 3;
            }
            set(&server.listening_loop_client_accept_failed, false);
        }

        for(int i = 0; i < server.rooms.n; i++) {
            Assert(server.room_ids[i] > 0); // Only positive numbers are allowed room IDs.
            
            Room *room = server.rooms.e + i;

            auto &clients = server.room_clients.e[i];

            // LISTEN TO WHAT THE CLIENTS HAVE TO SAY //
            for(int c = 0; c < clients.n; c++) {
                auto *client = &clients[c];

                while(true) {
                    bool error;
                    if(!platform_socket_has_bytes_to_read(&client->sock, &error)) { // TODO @Norelease: @Security: There has to be some limit to how much data a client can send, so that we can't be stuck in this loop forever!
                        if(error) {
                            disconnect_room_client(client); // @Cleanup @Boilerplate
                            c--;
                        }
                        break;
                    }

                    // @Cleanup

                    RSB_Packet_Header header;
                    RSB_Header(client, &header);
                    if(client == NULL) { c--; break; }

                    bool do_disconnect = false;

                    if(header.type == RSB_GOODBYE) {
                        // TODO @Norelease: Better logging, with more client info etc.
                        Debug_Print("Client (socket = %lld) sent goodbye message.\n", client->sock.handle);
                        do_disconnect = true;
                    }
                    else if(!read_and_handle_rsb_packet(client, header, room)) {
                        do_disconnect = true;
                    }
                    
                    if (do_disconnect) { // @Cleanup @Boilerplate
                        disconnect_room_client(client);
                        c--;
                        break;
                    }
                }
            }

            // RUN SIMULATION //
            update_room(room, i);
            //

            // SEND UPDATES TO CLIENTS //
            if(!room->did_change) continue;
            room->did_change = false;
            
            // @Speed: Loop over just the sockets, not the whole Client structs.
            // @Speed: Prepare the data to send (all packets combined, endiannessed etc) and then just write that to all the clients.
                
            for(int c = 0; c < clients.n; c++) {
                auto *client = &clients[c];
                RCB_Packet(client, Tiles_Changed, 0, room_size, room->shared.tiles);
                if(client == NULL) c--;
            }
        }

        add_new_room_clients(&server);
            
        platform_sleep_microseconds(1000);
    }

    platform_deinit_socket_use();
    
    return 0;
}
