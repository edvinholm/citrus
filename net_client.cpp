



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


bool talk_to_room_server(Network_Node *node, Client *client, Array<C_RS_Action, ALLOC_MALLOC> *action_queue,
                         bool *_server_said_goodbye)
{
    *_server_said_goodbye = false;

    auto *room = &client->room; // IMPORTANT: @Robustness: This is safe only because the room always is at the same place.

    // REMEMBER NOT TO USE TEMPORARY MEMORY BEFORE LOCKING THE MUTEX!!!
    // REMEMBER NOT TO USE TEMPORARY MEMORY BEFORE LOCKING THE MUTEX!!!
    // REMEMBER NOT TO USE TEMPORARY MEMORY BEFORE LOCKING THE MUTEX!!!
    
    // WRITE //
    // @Robustness: Since the room can be updated many times
    //              after the action is queued and before
    //              we lock the mutex here, we need to
    //              be careful here and check that entities
    //              etc still exists.
    //              Should we, instead of having an action queue,
    //              look at the state of the room/game here, to
    //              figure out what packets need to be sent?
    lock_mutex(client->mutex);
    {
        for(int i = 0; i < action_queue->n; i++)
        {
            auto &action = action_queue->e[i];
            switch(action.type) { // @Jai: #complete
                case C_RS_ACT_CLICK_TILE: {
                    auto &ct = action.click_tile;
                    Enqueue(RSB_CLICK_TILE, node, ct.tile_ix);
                } break;
                    
                case C_RS_ACT_PLAYER_ACTION: {
                    auto &act = action.player_action;
                    Enqueue(RSB_PLAYER_ACTION, node, act);
                } break;
                    
                case C_RS_ACT_PLAYER_ACTION_DEQUEUE: {
                    Enqueue(RSB_PLAYER_ACTION_DEQUEUE, node, action.player_action_dequeue.action_id);
                } break;

                case C_RS_ACT_CHAT: {                    
                    auto &chat  = action.chat;

                    auto *user = current_user(client);                    
                    Assert(user);
                    if(!user) break;

                    auto *draft = &user->chat_draft;
                    String message_text = { draft->e, draft->n };
                    
                    Enqueue(RSB_CHAT, node, message_text);
                    draft->n = 0;
                } break;

                default: Assert(false); break;
            }
        }
    }
    unlock_mutex(client->mutex);
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
                Debug_Print("The room server said goodbye.\n");
                *_server_said_goodbye = true;
            } break;

            case RCB_ROOM_INIT: {
                auto *p = &header.room_init;

                // REMEMBER NOT TO USE TEMPORARY MEMORY BEFORE LOCKING THE MUTEX!!!
                // REMEMBER NOT TO USE TEMPORARY MEMORY BEFORE LOCKING THE MUTEX!!!
                // REMEMBER NOT TO USE TEMPORARY MEMORY BEFORE LOCKING THE MUTEX!!!

                // NOTE: We only read the *shared* part of the entitites here, leaving the rest of the structs zeroed.
                


                { Scoped_Lock(client->mutex);

                    ///////////////////////////////////////////////
                    ///////////////////////////////////////////////
                    // NOTE: @Norelease: @Temporary: @Speed
                    //       See comment in ROOM_UPDATE.
                    ///////////////////////////////////////////////
                    ///////////////////////////////////////////////

                    Array<Entity, ALLOC_MALLOC> entities = { 0 }; // @Speed: Reuse some buffer.
                    array_add_uninitialized(entities, p->num_entities);
                    defer(clear(&entities););

                    for (int i = 0; i < p->num_entities; i++) {
                        Zero(entities[i]);
                        Read_To_Ptr(Entity, &entities[i], node);
                    }

                    for (int i = 0; i < entities.n; i++) {
                        auto *e = &entities[i];

                        if (e->type == ENTITY_PLAYER)
                        {
                            // @Volatile
                            // Copy path because it's temporary memory.

                            size_t path_size = sizeof(*e->player_e.walk_path) * e->player_e.walk_path_length;
                            v3 *path_copy = (v3 *)alloc(path_size, ALLOC_MALLOC);
                            memcpy(path_copy, e->player_e.walk_path, path_size);

                            e->player_e.walk_path = path_copy;
                        }

                    }

                    ///////////////////////////////////////////////
                    ///////////////////////////////////////////////
                    ///////////////////////////////////////////////
                    ///////////////////////////////////////////////


                    double t       = platform_get_time();
                    double world_t = world_time_for_room(room, t);
                    
                    room->t = p->time;
                    room->time_offset = room->t - t;
                    Debug_Print("Room time offset: %f\n", room->time_offset);
                    
                    static_assert(sizeof(Tile) == 1);
                    memcpy(room->tiles, p->tiles, room_size_x * room_size_y);
                    
                    static_assert(sizeof(room->walk_map.nodes[0]) == 1);
                    memcpy(room->walk_map.nodes, p->walk_map.nodes, room_size_x * room_size_y);

                    Assert(room->entities.n == 0);

                    array_set(room->entities, entities);

                    update_local_data_for_room(room, world_t, client);
                    room->static_geometry_up_to_date = false;
                }
                
            
            } break;
        
            case RCB_ROOM_UPDATE: {
                auto *p = &header.room_update;

                // REMEMBER NOT TO USE TEMPORARY MEMORY BEFORE LOCKING THE MUTEX!!!
                // REMEMBER NOT TO USE TEMPORARY MEMORY BEFORE LOCKING THE MUTEX!!!
                // REMEMBER NOT TO USE TEMPORARY MEMORY BEFORE LOCKING THE MUTEX!!!

                Fail_If_True(p->tile0 >= p->tile1);
            
                
                
                { Scoped_Lock(client->mutex);

                    ///////////////////////////////////////////////
                    ///////////////////////////////////////////////
                    // NOTE: @Norelease: @Temporary: @Speed
                    //       We can read everything from the node before locking the mutex.
                    //       But we want to be able to use temporary memory. So for now,
                    //       we read inside the mutex lock. We might want for @Jai before
                    //       changing this. We could maybe use the context in some way or
                    //       something. Or we make some memory buffer that the read code always 
                    //       uses...
                    ///////////////////////////////////////////////
                    ///////////////////////////////////////////////

                    // NOTE: We only read the *shared* part of the entitites here, leaving the rest of the structs zeroed.
                    Array<S__Entity, ALLOC_MALLOC> s_entities = {0}; // @Speed: Reuse some buffer.
                    array_add_uninitialized(s_entities, p->num_entities);
                    defer(clear(&s_entities););
                
                    for(int i = 0; i < p->num_entities; i++) {
                        Read_To_Ptr(Entity, &s_entities[i], node);
                        Assert(s_entities[i].type == ENTITY_PLAYER || s_entities[i].type == ENTITY_ITEM); // @Temporary: Should be checked in read_Entity()
                    }

                    Fail_If_True(p->num_chat_messages > MAX_CHAT_MESSAGES_PER_ROOM);
                    Chat_Message chat_messages[MAX_CHAT_MESSAGES_PER_ROOM];
                    for(int i = 0; i < p->num_chat_messages; i++)
                    {
                        auto *c = &chat_messages[i];
                        Read_To_Ptr(Chat_Message, c, node);
                    }

                    ///////////////////////////////////////////////
                    ///////////////////////////////////////////////
                    ///////////////////////////////////////////////
                    ///////////////////////////////////////////////


                    double t       = platform_get_time();
                    double world_t = world_time_for_room(room, t);
                    
                    remove_preview_entities(room);

                    // Reset exists_on_server
                    for(int i = 0; i < room->entities.n; i++) {
                        room->entities[i].exists_on_server = false;
                    }
                    
                    Tile *tiles = room->tiles;
                    for(int t = p->tile0; t < p->tile1; t++) {
                        tiles[t] = p->tiles[t - p->tile0];
                    }
                    
                    static_assert(sizeof(room->walk_map.nodes[0]) == 1);
                    memcpy(room->walk_map.nodes, p->walk_map_nodes, room_size_x * room_size_y);

                    for(int i = 0; i < s_entities.n; i++) {
                        
                        auto *e = find_or_add_entity(s_entities[i].id, room);
                        auto *s_e = static_cast<S__Entity *>(e);

                        Assert(s_entities[i].type == ENTITY_PLAYER || s_entities[i].type == ENTITY_ITEM); // @Temporary: Should be checked in read_Entity()
                        Assert(s_e->type == ENTITY_PLAYER || s_e->type == ENTITY_ITEM); // @Temporary: Should be checked in read_Entity()

                        if(s_entities[i].type == ENTITY_PLAYER)
                        {
                            if(e->player_e.walk_path != NULL)
                                dealloc(e->player_e.walk_path, ALLOC_MALLOC);
                        }
                        
                        *s_e = s_entities[i];
   
                        if(s_entities[i].type == ENTITY_PLAYER)
                        {
                            // @Volatile
                            // Copy path because it's temporary memory.
                            size_t path_size = sizeof(*e->player_e.walk_path) * e->player_e.walk_path_length;
                            v3 *path_copy = (v3 *)alloc(path_size, ALLOC_MALLOC);
                            memcpy(path_copy, e->player_e.walk_path, path_size);

                            e->player_e.walk_path = path_copy;
                        }
                        
                        e->exists_on_server = true;
                    }

                    // Remove entities with exists_on_server == false
                    for(int i = 0; i < room->entities.n; i++) {
                        auto *e = &room->entities[i];
                        if(e->exists_on_server) continue;

                        clear(e);
                        array_unordered_remove(room->entities, i);
                        i--;
                    }

                    // Chat messages
                    static_assert(sizeof(room->chat_messages) == sizeof(chat_messages));
                    for(int i = 0; i < p->num_chat_messages; i++)
                    {
                        auto *c = &chat_messages[i];
                        
                        if(i < room->num_chat_messages)
                            clear(&room->chat_messages[i].text, ALLOC_MALLOC);

                        // Copy the message text. (Otherwise it points to the network node receive buffer)
                        c->text = copy_of(&c->text, ALLOC_MALLOC);
                        room->chat_messages[i] = *c;
                    }
                    room->num_chat_messages = p->num_chat_messages;

                    // Tiles
                    if(p->tile1 - p->tile0 > 0) {
                        room->static_geometry_up_to_date = false;
                    }

                    update_local_data_for_room(room, world_t, client);
                }


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
    *_server_said_goodbye = false;

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
                Debug_Print("The user server said goodbye.\n");
                *_server_said_goodbye = true;
            } break;

            case UCB_USER_INIT: {
                auto *p = &header.user_init;

                Inventory_Slot inventory[ARRLEN(user->inventory)];
                for(int i = 0; i < ARRLEN(inventory); i++) {
                    Read_To_Ptr(Inventory_Slot, inventory + i, node);
                }
                
                lock_mutex(mutex);
                {
                    clear(&user->username, ALLOC_MALLOC);
                    
                    user->id        = p->id;
                    user->username  = copy_of(&p->username, ALLOC_MALLOC);
                    user->color     = p->color;
                    
                    user->money     = p->money;
                    user->reserved_money = p->reserved_money;
                    
                    static_assert(sizeof(inventory) == sizeof(user->inventory));
                    memcpy(user->inventory, inventory, sizeof(inventory));

                    user->initialized = true;
                }
                unlock_mutex(mutex);
            } break;

            case UCB_USER_UPDATE: {
                auto *p = &header.user_update;
                
                Inventory_Slot inventory[ARRLEN(user->inventory)];
                for(int i = 0; i < ARRLEN(inventory); i++) {
                    Read_To_Ptr(Inventory_Slot, inventory + i, node);
                }

                static_assert(sizeof(inventory) == sizeof(user->inventory));

                lock_mutex(mutex);
                {
                    clear(&user->username, ALLOC_MALLOC);
                    
                    user->id        = p->id;
                    user->username  = copy_of(&p->username, ALLOC_MALLOC);
                    user->color     = p->color;
                    
                    user->money     = p->money;
                    user->reserved_money = p->reserved_money;
                    
                    static_assert(sizeof(inventory) == sizeof(user->inventory));
                    memcpy(user->inventory, inventory, sizeof(inventory));
                }
                unlock_mutex(mutex);
                
            } break;
        };
        
    }
    
    return true;
}

bool talk_to_market_server(Network_Node *node, Client *client, Array<C_MS_Action, ALLOC_MALLOC> *action_queue,
                           bool *_server_said_goodbye)
{
    *_server_said_goodbye = false;

    // WRITE //
    lock_mutex(client->mutex); // @Speed: We might not need to lock the mutex here, but can do it only for the actions that need it, inside the loop. Because the passed action_queue is a copy of the shared one (2021-02-08).
    {
        for(int i = 0; i < action_queue->n; i++)
        {
            auto *action = &(*action_queue)[i];
            switch(action->type) { // @Jai: #complete
                
                case C_MS_PLACE_ORDER: {
                    auto *x = &action->place_order;
                    
                    Item_Type_ID item_type_to_buy = ITEM_NONE_OR_NUM;
                    Item_ID      item_id_to_sell = NO_ITEM;

                    if(x->is_buy_order) {
                        item_type_to_buy = x->buy.item_type;
                    } else {
                        item_id_to_sell = x->sell.item_id;
                    }
                    
                    Enqueue(MSB_PLACE_ORDER, node, x->price, x->is_buy_order, item_id_to_sell, item_type_to_buy);
                } break;

                case C_MS_SET_VIEW: {
                    auto *x = &action->set_view;
                    Enqueue(MSB_SET_VIEW_TARGET, node, x->target);
                } break;
                    
            }
        }
    }
    unlock_mutex(client->mutex);
    Fail_If_True(!send_outbound_packets(node));

    
    // READ //
    while(true) {
        
        bool error;
        MCB_Packet_Header header;
        if(!receive_next_mcb_packet(node, &header, &error)) {
            if(error) return false;
            break;
        }

        switch(header.type)
        {
            case MCB_GOODBYE: {
                // Kicked from the server :(
                Debug_Print("The market server said goodbye.\n");
                *_server_said_goodbye = true;
            } break;

            case MCB_MARKET_INIT: {
                auto *p = &header.market_init;
                
                lock_mutex(client->mutex);
                {
                    auto *market = &client->market;
                    
                    market->initialized = true;
                }
                unlock_mutex(client->mutex);
            } break;

            case MCB_MARKET_UPDATE: {
                auto *p = &header.market_update;

                lock_mutex(client->mutex);
                {
                    auto *market = &client->market;
                    auto *view = &market->view;

                    *static_cast<S__Market_View *>(view) = p->view;
                                
                    market->waiting_for_view_update = false;
                    market->ui_needs_update = true;
                }
                unlock_mutex(client->mutex);
                
            } break;
        };
        
    }
    
    return true;
}





// @Cleanup: @Jai: Make this a proc local to network_loop()
// TODO @Norelease: Disconnect from room server when disconnecting from user server...
//                  Should probably be a higher level logic thing, not in this proc.
bool client__disconnect_from_user_server(User_Server_Connection *us_con, bool say_goodbye = true)
{
    Assert(us_con->connected);
 
    Debug_Print("Disconnecting from user server...\n");
    bool result = disconnect_from_user_server(&us_con->node, say_goodbye);

    us_con->current_user = NO_USER;
    us_con->connected = false;

    return result;
}

// @Cleanup: @Jai: Make this a proc local to network_loop()
bool client__disconnect_from_room_server(Room_Server_Connection *rs_con, bool say_goodbye = true)
{
    Assert(rs_con->connected);
    
    Debug_Print("Disconnecting from room server...\n");

    bool result = disconnect_from_room_server(&rs_con->node, say_goodbye);
    
    rs_con->current_room = 0;
    rs_con->connected = false;

    return result;
}

// @Cleanup: @Jai: Make this a proc local to network_loop()
bool client__disconnect_from_market_server(Market_Server_Connection *ms_con, bool say_goodbye = true)
{
    Assert(ms_con->connected == true);
    
    Debug_Print("Disconnecting from market server...\n");
    bool result = disconnect_from_market_server(&ms_con->node, say_goodbye);

    ms_con->current_user = NO_USER;
    ms_con->connected = false;

    return result;
}



DWORD network_loop(void *loop_)
{
    Network_Loop *loop = (Network_Loop *)loop_;

    Client *client = loop->client;

    Room_Server_Connection   rs_connection;
    User_Server_Connection   us_connection;
    Market_Server_Connection ms_connection;

    // START INITIALIZATION //
    lock_mutex(client->mutex);
    {
        Assert(loop->state == Network_Loop::INITIALIZING);
        
        rs_connection = client->connections.room;
        us_connection = client->connections.user;
        ms_connection = client->connections.market;
        
        // INITIALIZATION DONE //
        loop->state = Network_Loop::RUNNING;
    }
    unlock_mutex(client->mutex);

    const Allocator_ID allocator = ALLOC_MALLOC;
    
    // ROOM SERVER
    bool room_connect_requested;
    Room_ID requested_room;
    Array<C_RS_Action, allocator> room_action_queue = { 0 };
    //

    // USER SERVER
    bool user_connect_requested;
    User_ID requested_user;
    //
    
    // MARKET SERVER
    bool market_connect_requested;
    Array<C_MS_Action, allocator> market_action_queue = { 0 };
    //
    
    while(true) {
        bool did_connect_to_room_this_loop   = false;
        bool did_connect_to_user_this_loop   = false;
        bool did_connect_to_market_this_loop = false;
        
        lock_mutex(client->mutex);
        {
            if(loop->state == Network_Loop::SHOULD_EXIT) {
                unlock_mutex(client->mutex);
                break;
            }

            // ROOM SERVER //
            room_connect_requested = client->connections.room_connect_requested;
            requested_room         = client->connections.requested_room;
            array_set(room_action_queue, client->connections.room_action_queue);
            client->connections.room_action_queue.n = 0;
            // // //

            // USER SERVER // // @Cleanup @Boilerplate
            user_connect_requested = client->connections.user_connect_requested;
            requested_user = client->connections.requested_user;
            // // //
            
            // MARKET SERVER // // @Cleanup @Boilerplate
            market_connect_requested = client->connections.market_connect_requested;
            array_set(market_action_queue, client->connections.market_action_queue);
            client->connections.market_action_queue.n = 0;
            // // //
            
            // Make sure no-one else has written to these structs -- Network Loop is the only one that is allowed to.
            Assert(equal(&client->connections.room, &rs_connection));
            Assert(totally_equal(&client->connections.user,   &us_connection));
            Assert(totally_equal(&client->connections.market, &ms_connection));
            // --
        }
        unlock_mutex(client->mutex);

        // TALK TO SERVERS HERE //
        {
            // CONNECT TO ROOM SERVER //
            if(room_connect_requested) {

                if(!us_connection.connected) {
                    Debug_Print("Can't connect to room server before connected to user server.\n");
                }
                else
                {                
                    if(rs_connection.connected) {
                        // DISCONNECT FROM CURRENT SERVER //
                        if(!client__disconnect_from_room_server(&rs_connection)) { Debug_Print("Disconnecting from room server failed. (%s:%d)\n", __FILE__, __LINE__); }
                        else Debug_Print("Disconnected from room server successfully. (%s:%d)\n", __FILE__, __LINE__);
                    }

                    Assert(!rs_connection.connected);

                    User_ID user_id = us_connection.current_user;
                    
                    if(connect_to_room_server(requested_room, user_id, &rs_connection.node)) {
                        rs_connection.connected = true;
                        rs_connection.current_room = requested_room;
                    
                        rs_connection.last_connect_attempt_failed = false;
                        did_connect_to_room_this_loop = true;
                    }
                    else
                    {
                        rs_connection.connected = false;
                        rs_connection.last_connect_attempt_failed = true;
                    }
                }
            }
            // // //
            
            // CONNECT TO USER SERVER // // @Cleanup @Boilerplate
            if(user_connect_requested) {

                if(us_connection.connected) {
                    // DISCONNECT FROM USER SERVER //
                    if(!client__disconnect_from_user_server(&us_connection)) { Debug_Print("Disconnecting from user server failed. (%s:%d)\n", __FILE__, __LINE__); }
                    else Debug_Print("Disconnected from user server successfully.\n");

                    // DISCONNECT FROM ROOM AND MARKET SERVERS, because they depend on the current user ID. //
                    if(rs_connection.connected) {
                        if(!client__disconnect_from_room_server(&rs_connection)) { Debug_Print("Disconnecting from room server failed. (%s:%d)\n", __FILE__, __LINE__); }
                        else Debug_Print("Disconnected from room server successfully. (%s:%d)\n", __FILE__, __LINE__);
                    }

                    if(ms_connection.connected) {
                        if(!client__disconnect_from_market_server(&ms_connection)) { Debug_Print("Disconnecting from market server failed. (%s:%d)\n", __FILE__, __LINE__); }
                        else Debug_Print("Disconnected from market server successfully. (%s:%d)\n", __FILE__, __LINE__);
                    }
                }

                Assert(!us_connection.connected);
                
                if(connect_to_user_server(requested_user, &us_connection.node, US_CLIENT_PLAYER, 0)) {
                    us_connection.connected = true;
                    us_connection.current_user = requested_user;
                    
                    us_connection.last_connect_attempt_failed = false;
                    did_connect_to_user_this_loop = true;

                    Debug_Print("Connected to user server for user ID = \"%llu\".\n", requested_user);
                }
                else
                {
                    us_connection.connected = false;
                    us_connection.last_connect_attempt_failed = true;
                }
            }
            // // //
                        
            // CONNECT TO MARKET SERVER // // @Cleanup @Boilerplate
            if(market_connect_requested) {
                if(!us_connection.connected) {
                    Debug_Print("Can't connect to market server before connected to user server.\n");
                }
                else
                {                
                    if(ms_connection.connected) {
                        // DISCONNECT FROM CURRENT SERVER //
                        if(!client__disconnect_from_market_server(&ms_connection)) { Debug_Print("Disconnecting from market server failed. (%s:%d)\n", __FILE__, __LINE__); }
                        else Debug_Print("Disconnected from market server successfully. (%s:%d)\n", __FILE__, __LINE__);
                    }

                    Assert(!ms_connection.connected);
                    
                    User_ID user_id = us_connection.current_user;
                
                    if(connect_to_market_server(user_id, &ms_connection.node, MS_CLIENT_PLAYER)) {
                        ms_connection.connected = true;
                        ms_connection.current_user = user_id;
                    
                        ms_connection.last_connect_attempt_failed = false;
                        did_connect_to_market_this_loop = true;

                        Debug_Print("Connected to market server for user ID = \"%llu\".\n", user_id);
                    }
                    else
                    {
                        ms_connection.connected = false;
                        ms_connection.last_connect_attempt_failed = true;
                    }
                }
            }
            // // //

            

            // TALK TO ROOM SERVER //
            if(rs_connection.connected &&
               !did_connect_to_room_this_loop)
            {
                // NOTE: talk_to_room_server might lock client->mutex.
                bool server_said_goodbye = false;
                bool talk = talk_to_room_server(&rs_connection.node, client,
                                                &room_action_queue, &server_said_goodbye);
                if(!talk || server_said_goodbye)
                {
                    //TODO @Norelease Notify Main Loop about if something failed or if server said goodbye.
                    bool say_goodbye = !server_said_goodbye;
                    client__disconnect_from_room_server(&rs_connection, say_goodbye);
                }
            }
            // // //

            // TALK TO USER SERVER // // @Cleanup @Boilerplate
            if(us_connection.connected &&
               !did_connect_to_user_this_loop)
            {
                bool server_said_goodbye = false;
                bool talk = talk_to_user_server(&us_connection.node, client->mutex, &client->user, &server_said_goodbye);
                if(!talk || server_said_goodbye)
                {
                    //TODO @Norelease Notify Main Loop about if something failed or if server said goodbye.
                    bool say_goodbye = !server_said_goodbye;
                    client__disconnect_from_user_server(&us_connection, say_goodbye);
                }
            }
            // // //

            // TALK TO MARKET SERVER // // @Cleanup @Boilerplate
            if(ms_connection.connected &&
               !did_connect_to_market_this_loop)
            {
                bool server_said_goodbye;
                bool talk = talk_to_market_server(&ms_connection.node, client, &market_action_queue, &server_said_goodbye);
                if(!talk || server_said_goodbye)
                {
                    //TODO @Norelease Notify Main Loop about if something failed or if server said goodbye.
                    bool say_goodbye = !server_said_goodbye;
                    client__disconnect_from_market_server(&ms_connection, say_goodbye);
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
            if(room_connect_requested)   client->connections.room_connect_requested   = false;
            if(user_connect_requested)   client->connections.user_connect_requested   = false;
            if(market_connect_requested) client->connections.market_connect_requested = false;

            client->connections.room   = rs_connection;
            client->connections.user   = us_connection;
            client->connections.market = ms_connection;

            if(did_connect_to_user_this_loop)   clear_and_reset(&client->user, ALLOC_MALLOC);
            if(did_connect_to_room_this_loop)   clear_and_reset(&client->room);
            if(did_connect_to_market_this_loop) clear_and_reset(&client->market);
        }
        unlock_mutex(client->mutex);

        platform_sleep_milliseconds(1);
    }


    Debug_Print("Shutting down network loop...\n");

    
    // DISCONNECT FROM ALL CONNECTED SERVERS //

    // Room Server
    if(rs_connection.connected) {
        if(!client__disconnect_from_room_server(&rs_connection)) { Debug_Print("Disconnecting from room server failed.\n"); }
        else {
            Debug_Print("Disconnected from room server successfully.\n");
        }
    }

    // User Server
    if(us_connection.connected) {
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


