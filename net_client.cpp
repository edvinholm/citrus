



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




#define RSB_Packet(Client_Ptr, Packet_Ident, ...)                       \
    enqueue_rsb_##Packet_Ident##_packet(&Client_Ptr->server_connections.rsb_queue, __VA_ARGS__)


#define RCB_Header(Socket_Ptr, Packet_Ident, ...)                      \
    if(!read_rcb_##Packet_Ident##_header(Socket_Ptr, __VA_ARGS__)) {   \
        Debug_Print("Failed to read RCB header.\n");                \
        return false;                                               \
    }                                                               \


bool talk_to_room_server(Network_Node *node, Mutex &mutex, Array<C_RS_Action, ALLOC_NETWORK> *action_queue,
                         Room *room, double t, bool *_server_said_goodbye)
{
    // WRITE //
    // @Robustness: Since the room can be updated many times
    //              after the action is queued and before
    //              we lock the mutex here, we need to
    //              be careful here and check that entities
    //              etc still exists.
    //              Should we, instead of having an action queue,
    //              look at the state of the room/game here, to
    //              figure out what packets need to be sent?
    lock_mutex(mutex);
    {
        for(int i = 0; i < action_queue->n; i++)
        {
            auto &action = action_queue->e[i];
            switch(action.type) {
                case C_RS_ACT_CLICK_TILE: {
                    auto &ct = action.click_tile;
                    Enqueue(RSB_CLICK_TILE, node, ct.tile_ix, ct.item_to_place);
                } break;
            }
        }
    }
    unlock_mutex(mutex);
    Fail_If_True(!send_outbound_packets(node));

    
    // READ //
    while(true) {

        bool error;
        RCB_Packet_Header header;
        if(!receive_next_rcb_packet(node, &header, &error)) {
            if(error) return false;
            break;
        }

        switch(header.type)
        {
            case RCB_GOODBYE: {
                // Kicked from the server :(
                Debug_Print("The server said goodbye.\n");
                *_server_said_goodbye = true;
            } break;

            case RCB_ROOM_INIT: {
                auto *p = &header.room_init;

                // NOTE: We only read the *shared* part of the entitites here, leaving the rest of the structs zeroed.

                Array<Entity, ALLOC_TMP> entities = {0};
                array_add_uninitialized(entities, p->num_entities);
                
                for(int i = 0; i < p->num_entities; i++) {
                    Zero(entities[i]);
                    Read_To_Ptr(Entity, &entities[i].shared, node);
                }

                lock_mutex(mutex);
                {
                    room->shared.t = p->time;
                    room->time_offset = room->shared.t - t;
                    
                    Assert(sizeof(Tile) == 1);
                    memcpy(room->shared.tiles, p->tiles, room_size_x * room_size_y);

                    array_set(room->entities, entities);
                    
                    room->static_geometry_up_to_date = false;
                }
                unlock_mutex(mutex);
            
            } break;
        
            case RCB_ROOM_UPDATE: {
                auto *p = &header.room_update;
            
                Fail_If_True(p->tile0 >= p->tile1);
            
                // NOTE: We only read the *shared* part of the entitites here, leaving the rest of the structs zeroed.
                Array<S__Entity, ALLOC_TMP> s_entities = {0};
                array_add_uninitialized(s_entities, p->num_entities);
                
                for(int i = 0; i < p->num_entities; i++) {
                    Read_To_Ptr(Entity, &s_entities[i], node);
                }
            
                lock_mutex(mutex);
                {
                    remove_preview_entities(room);
                    
                    Tile *tiles = room->shared.tiles;
                    for(int t = p->tile0; t < p->tile1; t++) {
                        tiles[t] = p->tiles[t - p->tile0];
                    }

                    for(int i = 0; i < s_entities.n; i++) {
                        auto *s_e = &s_entities[i];

                        auto *e = find_or_add_entity(s_e->id, room);
                        e->shared = *s_e;
                    }

                    if(p->tile1 - p->tile0 > 0) {
                        room->static_geometry_up_to_date = false;
                    }
                }
                unlock_mutex(mutex);
            } break;
            
            default: {
                Assert(false);
                return false;
            } break;
        }
    }

    if(*_server_said_goodbye) return true;

    return true;
}


#define USB_Packet(Client_Ptr, Packet_Ident, ...)                       \
    enqueue_usb_##Packet_Ident##_packet(&Client_Ptr->server_connections.usb_queue, __VA_ARGS__)


#define UCB_Header(Socket_Ptr, Packet_Ident, ...)                       \
    if(!read_ucb_##Packet_Ident##_header(Socket_Ptr, __VA_ARGS__)) {   \
        Debug_Print("Failed to read UCB header.\n");                \
        return false;                                               \
    }                                                               \


bool talk_to_user_server(Network_Node *node, Mutex &mutex, User *user, bool *_server_said_goodbye)
{
    // READ //
    while(true) {
        
        bool error;
        UCB_Packet_Header header;
        if(!receive_next_ucb_packet(node, &header, &error)) {
            if(error) return false;
            break;
        }

        switch(header.type)
        {
            case UCB_GOODBYE: {
                // Kicked from the server :(
                Debug_Print("The server said goodbye.\n");
                *_server_said_goodbye = true;
            } break;

            case UCB_USER_INIT: {
                auto *p = &header.user_init;

                Item inventory[ARRLEN(user->shared.inventory)];
                for(int i = 0; i < ARRLEN(inventory); i++) {
                    Read_To_Ptr(Item, inventory + i, node);
                }
                
                lock_mutex(mutex);
                {
                    clear(&user->shared.username, ALLOC_APP);
                    
                    user->shared.id        = p->id;
                    user->shared.username  = copy_of(&p->username, ALLOC_APP);
                    user->shared.color     = p->color;
                    memcpy(user->shared.inventory, inventory, sizeof(user->shared.inventory));
                }
                unlock_mutex(mutex);
            } break;

            case UCB_USER_UPDATE: {
                auto *p = &header.user_update;
                
                Item inventory[ARRLEN(user->shared.inventory)];
                for(int i = 0; i < ARRLEN(inventory); i++) {
                    Read_To_Ptr(Item, inventory + i, node);
                }

                lock_mutex(mutex);
                {
                    clear(&user->shared.username, ALLOC_APP);
                    
                    user->shared.id        = p->id;
                    user->shared.username  = copy_of(&p->username, ALLOC_APP);
                    user->shared.color     = p->color;
                    memcpy(user->shared.inventory, inventory, sizeof(user->shared.inventory));
                }
                unlock_mutex(mutex);
            } break;
        };
        
    }

    if(*_server_said_goodbye) return true;
    
    return true;
}





// @Cleanup: @Jai: Make this a proc local to network_loop()
// TODO @Norelease: Disconnect from room server when disconnecting from user server...
//                  Should probably be a higher level logic thing, not in this proc.
bool client__disconnect_from_user_server(User_Server_Connection *us_con, bool say_goodbye = true)
{
    Debug_Print("Disconnecting from user server...\n");
    bool result = disconnect_from_user_server(&us_con->node, say_goodbye);

    us_con->current_user = NO_USER;
    us_con->status = USER_SERVER_DISCONNECTED;

    return result;
}

// @Cleanup: @Jai: Make this a proc local to network_loop()
bool client__disconnect_from_room_server(Room_Server_Connection *rs_con, bool say_goodbye = true)
{
    Debug_Print("Disconnecting from room server...\n");

    bool result = disconnect_from_room_server(&rs_con->node, say_goodbye);
    
    rs_con->current_room = 0;
    rs_con->status = ROOM_SERVER_DISCONNECTED;

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
    Array<C_RS_Action, allocator> room_action_queue;
    //

    // USER SERVER
    bool user_connect_requested;
    User_ID requested_user;
    //
    
    while(true) {
        bool did_connect_to_room_this_loop = false;
        bool did_connect_to_user_this_loop = false;
        
        lock_mutex(client->mutex);
        {
            if(loop->state == Network_Loop::SHOULD_EXIT) {
                unlock_mutex(client->mutex);
                break;
            }

            // ROOM SERVER //
            room_connect_requested = client->server_connections.room_connect_requested;
            requested_room         = client->server_connections.requested_room;
            array_set(room_action_queue, client->server_connections.room_action_queue);
            client->server_connections.room_action_queue.n = 0;
            // // //

            // USER SERVER // // @Cleanup @Boilerplate
            user_connect_requested = client->server_connections.user_connect_requested;
            requested_user = client->server_connections.requested_user;
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

                if(us_connection.status != USER_SERVER_CONNECTED) {
                    Debug_Print("Can't connect to room server before connected to user server.\n");
                }
                else
                {                
                    if(rs_connection.status == ROOM_SERVER_CONNECTED) {
                        // DISCONNECT FROM CURRENT SERVER //
                        if(!client__disconnect_from_room_server(&rs_connection)) { Debug_Print("Disconnecting from room server failed.\n"); }
                        else {
                            Debug_Print("Disconnected from room server successfully.\n");
                        }
                    }

                    Assert(rs_connection.status == ROOM_SERVER_DISCONNECTED);

                    User_ID user_id = us_connection.current_user;
                    
                    if(connect_to_room_server(requested_room, user_id, &rs_connection.node)) {
                        rs_connection.status = ROOM_SERVER_CONNECTED;
                        rs_connection.current_room = requested_room;
                    
                        rs_connection.last_connect_attempt_failed = false;
                        did_connect_to_room_this_loop = true;
                    }
                    else
                    {
                        rs_connection.status = ROOM_SERVER_DISCONNECTED;
                        rs_connection.last_connect_attempt_failed = true;
                    }
                }
            }
            // // //
            
            // CONNECT TO USER SERVER // // @Cleanup @Boilerplate
            if(user_connect_requested) {

                if(us_connection.status == USER_SERVER_CONNECTED) {
                    // DISCONNECT FROM CURRENT SERVER //
                    if(!client__disconnect_from_user_server(&us_connection)) { Debug_Print("Disconnecting from user server failed.\n"); }
                    else {
                        Debug_Print("Disconnected from user server successfully.\n");
                    }
                }

                Assert(us_connection.status == USER_SERVER_DISCONNECTED);
                
                if(connect_to_user_server(requested_user, &us_connection.node, US_CLIENT_PLAYER)) {
                    us_connection.status = USER_SERVER_CONNECTED;
                    us_connection.current_user = requested_user;
                    
                    us_connection.last_connect_attempt_failed = false;
                    did_connect_to_user_this_loop = true;

                    Debug_Print("Connected to user server for user ID = \"%llu\".\n", requested_user);
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
                double t = platform_milliseconds() / 1000.0;
                
                bool server_said_goodbye;
                bool talk = talk_to_room_server(&rs_connection.node, client->mutex,
                                                &room_action_queue,
                                                &client->game.room, /* IMPORTANT: Passing a pointer to the room here only works because we only have one room, and it will always be at the same place in memory. */
                                                t,
                                                &server_said_goodbye);
                if(!talk || server_said_goodbye)
                {
                    //TODO @Norelease Notify Main Loop about if something failed or if server said goodbye.
                    bool say_goodbye = !server_said_goodbye;
                    client__disconnect_from_room_server(&rs_connection, say_goodbye);
                }
            }
            // // //

            // TALK TO USER SERVER // // @Cleanup @Boilerplate
            if(us_connection.status == USER_SERVER_CONNECTED &&
               !did_connect_to_user_this_loop)
            {
                bool server_said_goodbye;
                bool talk = talk_to_user_server(&us_connection.node, client->mutex, &client->user, &server_said_goodbye);
                if(!talk || server_said_goodbye)
                {
                    //TODO @Norelease Notify Main Loop about if something failed or if server said goodbye.
                    bool say_goodbye = !server_said_goodbye;
                    client__disconnect_from_user_server(&us_connection, say_goodbye);
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

        platform_sleep_milliseconds(1);
    }


    Debug_Print("Shutting down network loop...\n");

    
    // DISCONNECT FROM ALL CONNECTED SERVERS //

    // Room Server
    if(rs_connection.status == ROOM_SERVER_CONNECTED) {
        if(!client__disconnect_from_room_server(&rs_connection)) { Debug_Print("Disconnecting from room server failed.\n"); }
        else {
            Debug_Print("Disconnected from room server successfully.\n");
        }
    }

    // User Server
    if(us_connection.status == USER_SERVER_CONNECTED) {
        if(!client__disconnect_from_user_server(&us_connection)) { Debug_Print("Disconnecting from user server failed.\n"); }
        else {
            Debug_Print("Disconnected from user server successfully.\n");
        }
    }

    Debug_Print("End of network loop.\n");                
    
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


