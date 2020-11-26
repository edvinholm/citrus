
#define Enqueue(Type, Dest, Queue_Ptr) \
    enqueue_##Type(Dest, Queue_Ptr)

inline
void enqueue(void *data, u64 length, Packet_Queue *queue)
{
    array_add(*queue, (u8 *)data, length);
}

// @NoRelease
// TODO @Incomplete: On big-endian machines, transform to/from little-endian on send/receive.
//                   (We DO NOT use the 'network byte order' which is big-endian).
//                   This is for @Speed. Most machines we are targeting are little-endian.


inline
void enqueue_s8(s8 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_s16(s16 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_s32(s32 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_s64(s64 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_u8(u8 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_u16(u16 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_u32(u32 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

inline
void enqueue_u64(u64 i, Packet_Queue *queue)
{
    return enqueue(&i, sizeof(i), queue);
}

void enqueue_RSB_Packet_Header(RSB_Packet_Type type, Packet_Queue *queue)
{
    Enqueue(u64, type, queue);
}

void enqueue_rsb_Click_Tile_packet(Packet_Queue *queue, u64 tile_ix)
{
    Enqueue(RSB_Packet_Header, RSB_CLICK_TILE, queue);

    Enqueue(u64, tile_ix, queue);
}




struct Client;

struct Network_Loop
{
    enum State
    {
        INITIALIZING,
        RUNNING,
        SHOULD_EXIT
    };

    State  state;
    Thread thread;
    Client *client;
};


bool connect_to_room_server(Room_ID room_id, Socket *_socket)
{
    Socket sock;
    if(!platform_create_tcp_socket(&sock)) {
        Debug_Print("Unable to create socket.\n");
        return false;
    }

    if(!platform_connect_socket(&sock, "127.0.0.1", ROOM_SERVER_PORT)) {
        Debug_Print("Unable to connect socket to room server. WSA Error: %d\n", WSAGetLastError());
        return false;
    }

    Write(u64, room_id, &sock);

    platform_set_socket_read_timeout(&sock, 5 * 1000);
    
    Fail_If_True(read_room_connect_status_code(&sock) != ROOM_CONNECT__REQUEST_RECEIVED);
    Fail_If_True(read_room_connect_status_code(&sock) != ROOM_CONNECT__CONNECTED);

    platform_set_socket_read_timeout(&sock, 1000);
    
    *_socket = sock;
    
    return true;
}

bool disconnect_from_room_server(Room_Server_Connection *rs_con, bool say_goodbye = true)
{
    Debug_Print("Disconnecting from room server...\n");
    
    // IMPORTANT: REMEMBER: Even if we fail to say goodbye, always close the socket anyway.
    bool result = true;

    if(say_goodbye)
    {
        // REMEMBER: This is special, so don't do anything fancy here that can put us in an infinite disconnection loop.
        if(!write_rsb_Goodbye_packet(&rs_con->socket)) result = false;
        RCB_Packet_Header header;
        if(!read_RCB_Packet_Header(&header, &rs_con->socket) ||
           header.type != RCB_GOODBYE)
        {
            result = false;
        }
    }
    
    if(!platform_close_socket(&rs_con->socket)) result = false;

    rs_con->current_room = 0;
    rs_con->status = ROOM_SERVER_DISCONNECTED;
    
    return result;
}



#define RSB_Packet(Client_Ptr, Packet_Ident, ...)                       \
    enqueue_rsb_##Packet_Ident##_packet(&Client_Ptr->server_connections.rsb_queue, __VA_ARGS__)


#define RCB_Header(Socket_Ptr, Packet_Ident, ...)                       \
    if(!read_rcb_##Packet_Ident##_header(Socket_Ptr, __VA_ARGS__)) {   \
        Debug_Print("Failed to read RCB header.\n");                \
        return false;                                               \
    }                                                               \

bool read_and_handle_rcb_packet(Socket *sock, Mutex &mutex, Room *room, bool *_server_said_goodbye)
{
    Debug_Print("read_and_handle_rcb_packet\n");
    
    RCB_Packet_Header header;
    Read_To_Ptr(RCB_Packet_Header, &header, sock);

    *_server_said_goodbye = false;

    switch(header.type) {

        case RCB_GOODBYE: {
            // Kicked from the server :(
            Debug_Print("The server said goodbye.\n");
            *_server_said_goodbye = true;
        } break;

        case RCB_ROOM_INIT: {
            // @Temporary: Reuse some buffer. (WE CAN'T USE TEMPORARY MEMORY BECAUSE WE HAVE NOT LOCKED THE MUTEX AT THIS POINT)
            size_t rec_tiles_size = sizeof(Tile) * (room_size_x * room_size_y);
            Tile *rec_tiles;
            rec_tiles = (Tile *)malloc(rec_tiles_size);
            defer(free(rec_tiles););

            Assert(sizeof(Tile) == 1);
            Read_Bytes(rec_tiles, rec_tiles_size, sock);

            lock_mutex(mutex);
            {
                memcpy(room->shared.tiles, rec_tiles, rec_tiles_size);
            }
            unlock_mutex(mutex);
            
        } break;
        
        case RCB_TILES_CHANGED: {
            u64 tile0, tile1;
            RCB_Header(sock, Tiles_Changed, &tile0, &tile1);

            Fail_If_True(tile0 >= tile1);
            
            // @Temporary: Reuse some buffer. (WE CAN'T USE TEMPORARY MEMORY BECAUSE WE HAVE NOT LOCKED THE MUTEX AT THIS POINT)
            size_t rec_tiles_size = sizeof(Tile) * (tile1 - tile0);
            Tile *rec_tiles;
            rec_tiles = (Tile *)malloc(rec_tiles_size);
            defer(free(rec_tiles););

            //TODO @Speed: Make a Read_N, where we read N of type Type. So we can unroll that loop etc....
            {
#if 0
                for(int i = 0; i < tile1 - tile0; i++) {
                    Read(Tile, rec_tiles + i, sock);
                }
#else
                Assert(sizeof(Tile) == 1);
                Read_Bytes(rec_tiles, rec_tiles_size, sock);
#endif
            }
            
            lock_mutex(mutex);
            {
                Tile *tiles = room->shared.tiles;
                for(int t = tile0; t < tile1; t++)
                {
                    tiles[t] = rec_tiles[t-tile0];
                }
            }
            unlock_mutex(mutex);
        } break;
            
        default: {
            Assert(false);
            return false;
        } break;
    }

    return true;
}

bool talk_to_room_server(Socket *sock, Mutex &mutex, Room *room, Packet_Queue *queue, bool *_server_said_goodbye)
{
    // READ //
    while(true) {
        bool error;
        if(!platform_socket_has_bytes_to_read(sock, &error)) {
            if(error) return false;
            break;
        }
        
        if(!read_and_handle_rcb_packet(sock, mutex, room, _server_said_goodbye)) {
            // TODO @Norelease
            Debug_Print("Unable to read and handle RCB packet. What should we do?.\n");
            return false;
        }
        
        if(*_server_said_goodbye) break;
    }

    if(*_server_said_goodbye) return true;
    
    // WRITE //
    if(queue->n > 0) {
        if(!write_to_socket(queue->e, queue->n, sock)) {
            // TODO @Norelease
            Debug_Print("Unable to write enqueued RSB data. What should we do?.\n");
            return false;
        }
    }

    return true;
}


#define USB_Packet(Client_Ptr, Packet_Ident, ...)                       \
    enqueue_usb_##Packet_Ident##_packet(&Client_Ptr->server_connections.usb_queue, __VA_ARGS__)


#define UCB_Header(Socket_Ptr, Packet_Ident, ...)                       \
    if(!read_ucb_##Packet_Ident##_header(Socket_Ptr, __VA_ARGS__)) {   \
        Debug_Print("Failed to read UCB header.\n");                \
        return false;                                               \
    }                                                               \



bool read_and_handle_ucb_packet(Socket *sock, Mutex &mutex, User *user, bool *_server_said_goodbye)
{        
    UCB_Packet_Header header;
    Read_To_Ptr(UCB_Packet_Header, &header, sock);

    *_server_said_goodbye = false;

    switch(header.type) {

        case UCB_GOODBYE: {
            // Kicked from the server :(
            Debug_Print("The server said goodbye.\n");
            *_server_said_goodbye = true;
        } break;

        case UCB_USER_INIT: {
            String username;
            v4 color;
            UCB_Header(sock, User_Init, &username, &color);

            clear(&user->shared.username, ALLOC_APP);
            user->shared.username = copy_of(&username, ALLOC_APP); // @Speed
            user->shared.color    = color;
        } break;
            
        default: {
            Assert(false);
            return false;
        } break;
    }

    return true;
}



bool talk_to_user_server(Socket *sock, Mutex &mutex, User *user, Packet_Queue *queue, bool *_server_said_goodbye)
{
    // READ //
    while(true) {
        bool error;
        if(!platform_socket_has_bytes_to_read(sock, &error)) {
            if(error) return false;
            break;
        }
        
        if(!read_and_handle_ucb_packet(sock, mutex, user, _server_said_goodbye)) {
            // TODO @Norelease
            Debug_Print("Unable to read and handle UCB packet. What should we do?.\n");
            return false;
        }
        
        if(*_server_said_goodbye) break;
    }

    if(*_server_said_goodbye) return true;
    
    // WRITE //
    if(queue->n > 0) {
        if(!write_to_socket(queue->e, queue->n, sock)) {
            // TODO @Norelease
            Debug_Print("Unable to write enqueued USB data. What should we do?.\n");
            return false;
        }
    }

    return true;
}





bool connect_to_user_server(String username, Socket *_socket)
{
    Socket sock;
    if(!platform_create_tcp_socket(&sock)) {
        Debug_Print("Unable to create socket.\n");
        return false;
    }

    if(!platform_connect_socket(&sock, "127.0.0.1", USER_SERVER_PORT)) {
        Debug_Print("Unable to connect socket to user server. WSA Error: %d\n", WSAGetLastError());
        return false;
    }

    Write(String, username, &sock);

    platform_set_socket_read_timeout(&sock, 5 * 1000);

    Fail_If_True(read_user_connect_status_code(&sock) != USER_CONNECT__CONNECTED);

    platform_set_socket_read_timeout(&sock, 1000);
    
    *_socket = sock;

    Debug_Print("Connected to user server for username = \"%.*s\".\n", (int)username.length, username.data);
    
    return true;
}

bool disconnect_from_user_server(User_Server_Connection *us_con, bool say_goodbye = true)
{
    Debug_Print("Disconnecting from user server...\n");
    
    // IMPORTANT: REMEMBER: Even if we fail to say goodbye, always close the socket anyway.
    bool result = true;

    if(say_goodbye)
    {
        // REMEMBER: This is special, so don't do anything fancy here that can put us in an infinite disconnection loop.
        if(!write_usb_Goodbye_packet(&us_con->socket)) result = false;
        UCB_Packet_Header header;
        if(!read_UCB_Packet_Header(&header, &us_con->socket) ||
           header.type != UCB_GOODBYE)
        {
            result = false;
        }
    }
    
    if(!platform_close_socket(&us_con->socket)) result = false;

    us_con->current_username.n = 0;
    us_con->status = USER_SERVER_DISCONNECTED;
    
    return result;
}






DWORD network_loop(void *loop_)
{
    Network_Loop *loop = (Network_Loop *)loop_;

    Client *client = loop->client;

    Room_Server_Connection rs_connection;
    User_Server_Connection us_connection;

    // START INITIALIZATION //
    lock_mutex(client->mutex);
    {
        Assert(loop->state == Network_Loop::INITIALIZING);
        
        rs_connection = client->server_connections.room;
        us_connection = client->server_connections.user;
        
        // INITIALIZATION DONE //
        loop->state = Network_Loop::RUNNING;
    }
    unlock_mutex(client->mutex);

    const Allocator_ID allocator = ALLOC_NETWORK;
    
    // ROOM SERVER
    bool room_connect_requested;
    Room_ID requested_room;
    Packet_Queue rsb_queue;
    //

    // USER SERVER
    bool user_connect_requested;
    Array<u8, allocator> requested_username;
    Packet_Queue usb_queue;
    //
    
    while(true) {
        bool did_connect_to_room_this_loop = false;
        bool did_connect_to_user_this_loop = false;
        
        lock_mutex(client->mutex);
        {
            if(loop->state == Network_Loop::SHOULD_EXIT) {
                unlock_mutex(client->mutex);

                // DISCONNECT FROM ALL CONNECTED SERVERS //
                if(rs_connection.status == ROOM_SERVER_CONNECTED) {
                    if(!disconnect_from_room_server(&rs_connection)) { Debug_Print("Disconnecting from room server failed.\n"); }
                    else {
                        Debug_Print("Disconnected from room server successfully.\n");
                    }
                }

                // @Cleanup @Boilerplate
                if(us_connection.status == USER_SERVER_CONNECTED) {
                    if(!disconnect_from_user_server(&us_connection)) { Debug_Print("Disconnecting from user server failed.\n"); }
                    else {
                        Debug_Print("Disconnected from user server successfully.\n");
                    }
                }
                
                break;
            }

            // ROOM SERVER //
            room_connect_requested = client->server_connections.room_connect_requested;
            requested_room         = client->server_connections.requested_room;

            array_set(rsb_queue, client->server_connections.rsb_queue);
            client->server_connections.rsb_queue.n = 0;
            // // //

            // USER SERVER // // @Cleanup @Boilerplate
            user_connect_requested = client->server_connections.user_connect_requested;
            array_set(requested_username, client->server_connections.requested_username);

            array_set(usb_queue, client->server_connections.usb_queue);
            client->server_connections.usb_queue.n = 0;
            // // //
            
            // Make sure no-one else has written to these structs -- Network Loop is the only one that is allowed to.
            Assert(equal(&client->server_connections.room, &rs_connection));
            Assert(totally_equal(&client->server_connections.user, &us_connection));
            // --
        }
        unlock_mutex(client->mutex);

        // TALK TO SERVERS HERE //
        {
            // CONNECT TO ROOM SERVER //
            if(room_connect_requested) {

                if(rs_connection.status == ROOM_SERVER_CONNECTED) {
                    // DISCONNECT FROM CURRENT SERVER //
                    if(!disconnect_from_room_server(&rs_connection)) { Debug_Print("Disconnecting from room server failed.\n"); }
                    else {
                        Debug_Print("Disconnected from room server successfully.\n");
                    }
                }

                Assert(rs_connection.status == ROOM_SERVER_DISCONNECTED);
                
                Socket sock;
                if(connect_to_room_server(requested_room, &sock)) {
                    rs_connection.status = ROOM_SERVER_CONNECTED;
                    rs_connection.current_room = requested_room;
                    rs_connection.socket = sock;
                    
                    rs_connection.last_connect_attempt_failed = false;
                    did_connect_to_room_this_loop = true;
                }
                else
                {
                    rs_connection.status = ROOM_SERVER_DISCONNECTED;
                    rs_connection.last_connect_attempt_failed = true;
                }
            }
            // // //
            
            // CONNECT TO USER SERVER // // @Cleanup @Boilerplate
            if(user_connect_requested) {

                if(us_connection.status == USER_SERVER_CONNECTED) {
                    // DISCONNECT FROM CURRENT SERVER //
                    if(!disconnect_from_user_server(&us_connection)) { Debug_Print("Disconnecting from user server failed.\n"); }
                    else {
                        Debug_Print("Disconnected from user server successfully.\n");
                    }
                }

                Assert(us_connection.status == USER_SERVER_DISCONNECTED);
                
                Socket sock;
                if(connect_to_user_server({requested_username.e, requested_username.n}, &sock)) {
                    us_connection.status = USER_SERVER_CONNECTED;
                    array_set(us_connection.current_username, requested_username);
                    us_connection.socket = sock;
                    
                    us_connection.last_connect_attempt_failed = false;
                    did_connect_to_user_this_loop = true;
                }
                else
                {
                    us_connection.status = USER_SERVER_DISCONNECTED;
                    us_connection.last_connect_attempt_failed = true;
                }
            }
            // // //

            // TALK TO ROOM SERVER //
            if(rs_connection.status == ROOM_SERVER_CONNECTED &&
               !did_connect_to_room_this_loop)
            {
                bool server_said_goodbye;
                bool talk = talk_to_room_server(&rs_connection.socket, client->mutex, &client->game.room, /* IMPORTANT: Passing a pointer to the room here only works because we only have one room, and it will always be at the same place in memory. */
                                                &rsb_queue, &server_said_goodbye);
                if(!talk || server_said_goodbye)
                {
                    //TODO @Norelease Notify Main Loop about if something failed or if server said goodbye.
                    bool say_goodbye = !server_said_goodbye;
                    disconnect_from_room_server(&rs_connection, say_goodbye);
                }
            }
            // // //

            // TALK TO USER SERVER // // @Cleanup @Boilerplate
            if(us_connection.status == USER_SERVER_CONNECTED &&
               !did_connect_to_user_this_loop)
            {
                bool server_said_goodbye;
                bool talk = talk_to_user_server(&us_connection.socket, client->mutex, &client->user, &usb_queue, &server_said_goodbye);
                if(!talk || server_said_goodbye)
                {
                    //TODO @Norelease Notify Main Loop about if something failed or if server said goodbye.
                    bool say_goodbye = !server_said_goodbye;
                    disconnect_from_user_server(&us_connection, say_goodbye);
                }
            }
            // // //
            
        }
        // //////////////////// //
        
        lock_mutex(client->mutex);
        {
            // Update Network Info

            // IMPORTANT that we do this 'if' here. Otherwise the main loop can set this var to true
            //           between the time we saw it was false and now. And then we would ignore the request.
            //           (It is not allowed to request a connection if one is already requested)
            if(room_connect_requested) client->server_connections.room_connect_requested = false;
            if(user_connect_requested) client->server_connections.user_connect_requested = false;

            client->server_connections.room = rs_connection;
            client->server_connections.user = us_connection;

            if(did_connect_to_room_this_loop) {
                reset(&client->game.room);
            }
        }
        unlock_mutex(client->mutex);
    }
    
    return 0;
}


// NOTE: Assumes you've zeroed *_loop
bool start_network_loop(Network_Loop *_loop, Client *client)
{
    _loop->state = Network_Loop::INITIALIZING;
    _loop->client = client;

    // Start thread
    if(!create_thread(&network_loop, _loop, &_loop->thread)) {
        return false;
    } 

    // Wait for the loop to initialize itself.
    while(true) {
        lock_mutex(client->mutex);
        defer(unlock_mutex(client->mutex););

        if(_loop->state != Network_Loop::INITIALIZING) {
            Assert(_loop->state == Network_Loop::RUNNING);
            break;
        }
    }

    return true;
}

void stop_network_loop(Network_Loop *loop, Client *client)
{
    lock_mutex(client->mutex);
    Assert(loop->state == Network_Loop::RUNNING);
    loop->state = Network_Loop::SHOULD_EXIT;
    unlock_mutex(client->mutex);

    join_thread(loop->thread);
}


