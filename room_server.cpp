
const char *LOG_TAG_RS      = ":RS:"; // Room Server
const char *LOG_TAG_RS_LIST = ":RS:LIST:"; // Listening loop

#define RS_Log_T(Tag, ...) \
    printf("[%s]", LOG_TAG_RS);                            \
    Log_T(Tag, __VA_ARGS__)
#define RS_Log(...)                     \
    Log_T(LOG_TAG_RS, __VA_ARGS__)
#define RS_Log_No_T(...)                     \
    Log(__VA_ARGS__)

#define RS_LIST_Log_T(Tag, ...) \
    printf("[%s]", LOG_TAG_RS_LIST);      \
    Log_T(Tag, __VA_ARGS__)
#define RS_LIST_Log(...)                     \
    Log_T(LOG_TAG_RS_LIST, __VA_ARGS__)
#define RS_LIST_No_T(...)                     \
    Log(__VA_ARGS__)


void create_dummy_entities(Room *room)
{
    v3 pp = { 0, 0 };
    for(int i = 0; i < 4; i++)
    {
        Item_Type_ID item_type_id = (Item_Type_ID)random_int(0, ITEM_NONE_OR_NUM - 1);
        Item_Type *item_type = item_types + item_type_id;
        
        S__Entity e = {0};
        e.type = ENTITY_ITEM;
        e.p = pp;
        e.p.x += item_type->volume.x * 0.5f;
        e.p.y += item_type->volume.y * 0.5f + (i % 2);
        
        e.item_type = item_type_id;

        Assert(room->num_entities < ARRLEN(room->entities));
        room->entities[room->num_entities++] = { e };

        pp.x += item_type->volume.x + 1;
    }
}

void create_dummy_rooms(Room_Server *server)
{
    double t = get_time();

    Array<Room_Client, ALLOC_APP> empty_client_array = {0};
    
    for(int i = 0; i < 8; i++) {
        Room room = {0};
        room.t = t;
        room.randomize_cooldown = random_float() * 3.0;

        create_dummy_entities(&room);
        
        array_add(server->room_ids, i + 1);
        array_add(server->rooms,    room);
        
        array_add(server->clients, empty_client_array);
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

        if(room->num_entities > 0) {
            auto *e = room->entities + random_int(0, room->num_entities-1);
            e->shared.p.xy = { (float)random_int(0, room_size_x), (float)random_int(0, room_size_y) };
            
            room->did_change = true;
        }
        
        
        Tile *tiles = room->shared.tiles;
        Tile *at  = tiles;
        Tile *end = tiles + room_size_x * room_size_y;
        while(at < end) {
            
            if(*at == TILE_WATER) {
                
                int y = (at - tiles) / room_size_x;
                int x = (at - tiles) % room_size_x;

#if 0
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
#endif
                
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


// TODO @Norelease @Robustness @Speed: This thing should have enough information to dismiss clients with invalid room IDs or login credentials.
DWORD room_server_listening_loop(void *room_server_)
{    
    auto *server = (Room_Server *)room_server_;
    auto *queue = &server->client_queue;

    Listening_Loop *loop = &server->listening_loop;

    RS_LIST_Log("Running.\n");

    bool   client_accepted;
    Socket client_socket;
    while(listening_loop_running(loop, &client_accepted, &client_socket, LOG_TAG_RS_LIST))
    {
        if(!client_accepted) continue;
        
        RS_LIST_Log("Client accepted.\n");

        // @Cleanup close socket boilerplate...
        // @Cleanup close socket boilerplate...
        // @Cleanup close socket boilerplate...

        if(!platform_set_socket_read_timeout(&client_socket, 1000)) {
            RS_LIST_Log("Unable to set read timeout for new client's socket (Last WSA Error: %d).\n", WSAGetLastError());
            
            if(!platform_close_socket(&client_socket)) {
                RS_LIST_Log("Unable to close new client's socket.\n");
                continue;
            }
            RS_LIST_Log("New client's socket closed successfully.\n");
            continue;
        }

        u64 requested_room_id;
        if(!read_u64(&requested_room_id, &client_socket)) {
            RS_LIST_Log("Unable to read requested room ID from new client.\n");

            if(!platform_close_socket(&client_socket)) {
                RS_LIST_Log("Unable to close new client's socket.\n");
                continue;
            }
            RS_LIST_Log("New client's socket closed successfully.\n");
            continue;
        }

        // TODO @Cleanup? Is this necessary?
        if(!write_room_connect_status_code(ROOM_CONNECT__REQUEST_RECEIVED, &client_socket)) {
            RS_LIST_Log("Unable to write status.\n");
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
                platform_sleep_milliseconds(1);
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

        platform_sleep_milliseconds(1);
    }


    RS_LIST_Log("Exiting.\n");
    return 0;
}

void disconnect_room_client(Room_Client *client)
{
    auto client_copy = *client;
    
    bool found_room = false;
    
    auto *server = client->server;
    Assert(server->room_ids.n == server->clients.n);
    for(int i = 0; i < server->room_ids.n; i++)
    {
        if(server->room_ids[i] == client->room) {
            auto *clients = &server->clients[i];
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
        RS_Log("Failed to write RCB Goodbye.\n");
    }
    
    if(!platform_close_socket(&client->sock)) {
        RS_Log("Failed to close room client socket.\n");
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
        RS_Log("Client disconnected from room %d due to RCB packet failure (socket = %lld, WSA Error: %d).\n", Room_Client_Ptr->room, Room_Client_Ptr->sock.handle, WSAGetLastError()); \
        disconnect_room_client(Room_Client_Ptr);                      \
        Room_Client_Ptr = NULL;                                         \
    }


// Receives an RSB packet if Room_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// Room_Client_Ptr is set to NULL.
#define RSB_Header(Room_Client_Ptr, Header_Ptr)                       \
    if((Room_Client_Ptr) &&                                             \
       !read_RSB_Packet_Header(Header_Ptr, &Room_Client_Ptr->sock)) {    \
        RS_Log("Client disconnected from room %d due to RSB header failure (socket = %lld, WSA Error: %d).\n", Room_Client_Ptr->room, Room_Client_Ptr->sock.handle, WSAGetLastError()); \
        disconnect_room_client(Room_Client_Ptr);                        \
        Room_Client_Ptr = NULL;                                         \
    }                                                               \



bool initialize_room_client(Room_Client *client, Room *room)
{
    RCB_Packet(client, Room_Init, room->shared.tiles, room->num_entities, room->entities);
    return true;
}

void add_new_room_clients(Room_Server *server)
{
    auto *queue = &server->client_queue;
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
                RS_Log("Failed to write ROOM_CONNECT__CONNECTED to client (socket = %lld).\n", client->sock.handle);

                // @Cleanup @Boilerplate
                platform_close_socket(&client->sock);
                clear(client);
                continue;
            }
                 
            auto *clients = &server->clients[room_index];
            client = array_add(*clients, *client);
            RS_Log("Added client (socket = %lld) to room %d.\n", client->sock.handle, client->room);       

            Assert(room != NULL);
            if(!initialize_room_client(client, room)) {
                // @Cleanup @Boilerplate
                platform_close_socket(&client->sock);
                clear(client);
                continue;
            }
        }

        queue->num_clients = 0;
    }
    unlock_mutex(queue->mutex);
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
            RS_Log("Client (socket = %lld) sent invalid RSB packet type (%u).\n", client->sock.handle, header.type);
            return false;
        } break;
    }
    
    return true;
}



bool start_room_server_listening_loop(Room_Server *server, Thread *thread)
{
    return start_listening_loop(&server->listening_loop, ROOM_SERVER_PORT,
                                &room_server_listening_loop, server, thread,
                                LOG_TAG_RS);
}

// REMEMBER to init_room_server before starting this.
//          IMPORTANT: Do NOT deinit_room_server after
//                     this is done -- this proc will do that for you.
DWORD room_server_main_loop(void *server_)
{
    Room_Server *server = (Room_Server *)server_;
    auto *listening_loop = &server->listening_loop;
    
    defer(deinit_room_server(server););
    
    RS_Log("Running.\n");
    
    create_dummy_rooms(server);

    // START LISTENING LOOP //
    Thread listening_loop_thread;
    if(!start_room_server_listening_loop(server, &listening_loop_thread)) {
        RS_Log("Failed to start listening loop.");
        return 1; // TODO @Norelease: Main server program must know about this!
    }
    // 
           
    double t = get_time();

    Array<int, ALLOC_APP> clients_to_disconnect = {0};
    while(!get(&server->should_exit)) {

        if(get(&listening_loop->client_accept_failed)){
            // RESTART LISTENING LOOP //
            RS_Log("Listening loop failed. Joining thread...\n");
            platform_join_thread(listening_loop_thread);
            
            RS_Log("Restarting listening loop...\n");
            deinit_listening_loop(&server->listening_loop);
            Zero(server->listening_loop);
            if(!start_room_server_listening_loop(server, &listening_loop_thread)) {
                RS_Log("Failed to restart listening loop.\n");
                // TODO @Norelease Disconnect all clients. (Say goodbye etc)
                // TODO @Norelease: Main server program must know about this!
                return 1;
            }
        }

        Assert(server->clients.n == server->rooms.n);
        for(int i = 0; i < server->rooms.n; i++) {
            Assert(server->room_ids[i] > 0); // Only positive numbers are allowed room IDs.
            
            Room *room = server->rooms.e + i;

            auto &clients = server->clients.e[i];

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
                        RS_Log("Client (socket = %lld) sent goodbye message.\n", client->sock.handle);
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

            // TODO @Norelease: Only send tiles/entities that changed.
            for(int c = 0; c < clients.n; c++) {
                auto *client = &clients[c];
                RCB_Packet(client, Room_Changed, 0, room_size, room->shared.tiles, room->num_entities, room->entities);
                if(client == NULL) c--;
            }
        }

        add_new_room_clients(server);
            
//        platform_sleep_milliseconds(1);
    }

    RS_Log("Stopping listening loop...");
    stop_listening_loop(listening_loop, &listening_loop_thread);
    RS_Log_No_T("Done.\n");

    // TODO @Incomplete: Disconnect all clients here, after exiting listening loop

    RS_Log("Exiting.\n");

    return 0;
}
