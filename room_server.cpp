
#include "room_server_bound.cpp"

#define RCB_OUTBOUND
#include "room_client_bound.cpp"


const int MAX_INBOUND_RS_CLIENT_PACKETS_PER_LOOP = 16;

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
        
        Item item = {0};
        item.id = random_int(999, 9999);// NOTE: We don't assign a unique ID to the item here, but this is just @Temporary stuff so it doesn't matter.
        item.type = item_type_id; 

        Entity e = {0};
        *static_cast<S__Entity *>(&e) = create_item_entity(&item, pp, room->t);
        e.id = 1 + room->next_entity_id_minus_one++;
        e.item_e.p.x += item_type->volume.x * 0.5f;
        e.item_e.p.y += item_type->volume.y * 0.5f + (i % 2);

        Assert(room->num_entities < ARRLEN(room->entities));
        room->entities[room->num_entities++] = { e };

        pp.x += item_type->volume.x + 1;
    }
}

void create_dummy_rooms(Room_Server *server)
{
    double t = get_time();

    Array<RS_Client, ALLOC_APP> empty_client_array = {0};
    
    for(int i = 0; i < 8; i++) {
        Room room = {0};
        room.t = get_time();
        room.randomize_cooldown = random_float() * 3.0;

        create_dummy_entities(&room);

#if 0
        for(int t = 0; t < room_size_x * room_size_y; t++) {
            room.tiles[t] = (Tile)random_int(0, TILE_NONE_OR_NUM-1);
        }
#else
        for(int y = 0; y < room_size_y; y++) {
            for(int x = 0; x < room_size_x; x++) {

                v2 tp = { (float)x, (float)y };
                v2 c  = { room_size_x / 2.0f, room_size_y / 2.0f };

                Tile t = TILE_GRASS;
                if(y == room_size_y-1 ||
                   x == room_size_x-1)
                {
                    t = TILE_WALL;
                }
                else if(magnitude(tp - c) > room_size_x * 0.5f * 0.9f)
                {
                    t = TILE_WATER;
                }
                else if (magnitude(tp - c) > room_size_x * 0.5f * 0.8f)
                {
                    t = TILE_SAND;
                }

                room.tiles[y * room_size_x + x] = t;
            }
        }
#endif
        
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



bool receive_next_rsb_packet(Network_Node *node, RSB_Packet_Header *_packet_header, bool *_error, bool block = false)
{
    if(!receive_next_network_node_packet(node, _error, block)) return false;
    
    *_error = true;
    Read_To_Ptr(RSB_Packet_Header, _packet_header, node);
    *_error = false;
    
    return true;
}

bool expect_type_of_next_rsb_packet(RSB_Packet_Type expected_type, Network_Node *node, RSB_Packet_Header *_packet_header)
{
    bool error;
    if(!receive_next_rsb_packet(node, _packet_header, &error, true)) return false;
    if(_packet_header->type != expected_type) return false;
    return true;
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
    while(listening_loop_running(loop, true, &client_accepted, &client_socket, LOG_TAG_RS_LIST))
    {
        if(!client_accepted) continue;
        
        RS_LIST_Log("Client accepted.\n");

        bool success = true;
        
        if(success && !platform_set_socket_read_timeout(&client_socket, 1000)) {
            RS_LIST_Log("Unable to set read timeout for new client's socket (Last WSA Error: %d).\n", WSAGetLastError());
            success = false;
        }

        Network_Node new_node = {0};
        reset_network_node(&new_node, client_socket);

        RSB_Packet_Header header;
        if(success && !expect_type_of_next_rsb_packet(RSB_HELLO, &new_node, &header)) {
            RS_LIST_Log("First packet did not have the expected type RSB_HELLO.\n");
            success = false;
        }

        // TODO @Cleanup? Is this necessary?
        if(success && !send_RCB_HELLO_packet_now(&new_node, ROOM_CONNECT__REQUEST_RECEIVED)) {
            RS_LIST_Log("Unable to write HELLO packet with REQUEST_RECEIVED.\n");
            success = false;
        }
        
        if(!success) {
            RS_LIST_Log("Client accept failed. Disconnecting socket.\n");
            if(!platform_close_socket(&client_socket)) {
                RS_LIST_Log("Unable to close new client's socket.\n");
                continue;
            }
            RS_LIST_Log("New client's socket closed successfully.\n");
            continue;
        }

        auto *hello = &header.hello;

        // ADD CLIENT TO QUEUE //
        RS_Client client = {0};
        client.node = new_node;
        client.room = hello->room;
        client.user = hello->as_user;
        client.server = server;

        lock_mutex(queue->mutex);
        {
            Assert(queue->count <= ARRLEN(queue->clients));
            
            // If the queue is full, wait..
            while(queue->count == ARRLEN(queue->clients))
            {
                unlock_mutex(queue->mutex);
                {
                    platform_sleep_milliseconds(1);
                }
                lock_mutex(queue->mutex);
            }
            
            Assert(queue->count <= ARRLEN(queue->clients));
                
            queue->clients[queue->count++] = client;
        }
        unlock_mutex(queue->mutex);
        // /// ///// // ///// //

        platform_sleep_milliseconds(1);
    }


    RS_LIST_Log("Exiting.\n");
    return 0;
}

void disconnect_room_client(RS_Client *client)
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
    if(!send_RCB_GOODBYE_packet_now(&client->node)) {
        RS_Log("Failed to send RCB_GOODBYE.\n");
    }
    
    if(!platform_close_socket(&client->node.socket)) {
        RS_Log("Failed to close room client socket.\n");
    }

    // IMPORTANT that we clear the copy here. Otherwise we would clear the client that is placed where our client was in the array.
    clear(&client_copy);
}


// Sends an RCB packet if RS_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// RS_Client_Ptr is set to NULL.
#define RCB_Packet(RS_Client_Ptr, Packet_Ident, ...)                    \
    if(RS_Client_Ptr) {                                                 \
        bool success = true;                                            \
        if(success && !enqueue_RCB_##Packet_Ident##_packet(&RS_Client_Ptr->node, __VA_ARGS__)) success = false; \
        Assert(RS_Client_Ptr->node.packet_queue.n == 1);                \
        if(success && !send_outbound_packets(&RS_Client_Ptr->node)) success = false; \
        if(!success) {                                                  \
            RS_Log("Client disconnected from room %d due to RCB packet failure (socket = %lld, WSA Error: %d).\n", RS_Client_Ptr->room, RS_Client_Ptr->node.socket.handle, WSAGetLastError()); \
            disconnect_room_client(RS_Client_Ptr);                      \
            RS_Client_Ptr = NULL;                                       \
        }                                                               \
    }



// Receives an RSB packet if RS_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// RS_Client_Ptr is set to NULL.
#define RSB_Header(RS_Client_Ptr, Header_Ptr)                       \
    if((RS_Client_Ptr) &&                                             \
       !read_RSB_Packet_Header(Header_Ptr, &RS_Client_Ptr->node)) {    \
        RS_Log("Client disconnected from room %d due to RSB header failure (socket = %lld, WSA Error: %d).\n", RS_Client_Ptr->room, RS_Client_Ptr->node.socket.handle, WSAGetLastError()); \
        disconnect_room_client(RS_Client_Ptr);                        \
        RS_Client_Ptr = NULL;                                         \
    }


bool initialize_room_client(RS_Client *client, Room *room)
{
    RCB_Packet(client, ROOM_INIT, room->t, room->num_entities, room->entities, room->tiles);
    return true;
}

void add_new_room_clients(Room_Server *server)
{
    auto *queue = &server->client_queue;
    lock_mutex(queue->mutex);
    {
        Assert(queue->count <= ARRLEN(queue->clients));

        for(int c = 0; c < queue->count; c++) {

            RS_Client *client = queue->clients + c;
                
            Room *room = NULL;
            
            int room_index = -1;
            for(int r = 0; r < server->room_ids.n; r++) {
                if(server->room_ids[r] == client->room) {
                    room_index = r;
                    room = server->rooms.e + r;
                    break;
                }
            }

            bool success = true;

            if(success && room_index == -1) {
                send_RCB_HELLO_packet_now(&client->node, ROOM_CONNECT__INVALID_ROOM_ID);
                success = false;
            }

            if(success && !send_RCB_HELLO_packet_now(&client->node, ROOM_CONNECT__CONNECTED))
            {
                RS_Log("Failed to write ROOM_CONNECT__CONNECTED to client (socket = %lld).\n", client->node.socket.handle);
                success = false;
            }
            
            if(success) {
                Assert(room != NULL);
                success = initialize_room_client(client, room);
            }

            if(!success) {
                // @Cleanup @Boilerplate                
                // IMPORTANT: Do not use disconnect_room_client here, because it won't work before we've added the client to the room client list.
                platform_close_socket(&client->node.socket);
                clear(client);
                
                continue;
            }

            Assert(room       != NULL);
            Assert(room_index != -1);
            auto *clients = &server->clients[room_index];
            client = array_add(*clients, *client);
            client->room_t_on_connect = room->t;
            RS_Log("Added client (socket = %lld) to room %d.\n", client->node.socket.handle, client->room);

            // @Norelease: @Temporary: We should not let players connect if there
            //                         is no entity with their ID.
            //                         Player entities should be added on request from
            //                         User Server or another Room Server.
            // MAKE SURE THERE IS A PLAYER ENTITY //
            bool player_entity_exists = false;
            for(int i = 0; i < room->num_entities; i++) {
                auto *e = &room->entities[i];
                if(e->type != ENTITY_PLAYER) continue;

                auto *p = &e->player_e;
                if(p->user_id == client->user) {
                    player_entity_exists = true;
                    break;
                }
            }

            if(!player_entity_exists) {
                Entity e = {0};
                e.id     = 1 + room->next_entity_id_minus_one++;
                e.type   = ENTITY_PLAYER;
                
                v3 p = { (float)random_int(1, room_size_x-1), (float)random_int(1, room_size_y-1), 0 };

                auto *player_e = &e.player_e;
                player_e->user_id     = client->user;
                player_e->walk_p0 = p;
                player_e->walk_p1 = p;
                
                Assert(room->num_entities < MAX_ENTITIES_PER_ROOM); // @Norelease: @Temporary: We should not add the entity here anyway (see comment above)
                room->entities[room->num_entities++] = e;

                room->did_change = true;
            }
            // ///////////////// //
        }

        queue->count = 0;
    }
    unlock_mutex(queue->mutex);
}

RS_User_Server_Connection *find_or_add_connection_to_user_server(User_ID user_id, Room_Server *server)
{
    const Allocator_ID allocator = ALLOC_NETWORK;
    
    for(int i = 0; i < server->user_server_connections.n; i++)
    {
        auto *connection = &server->user_server_connections[i];
        if(connection->user_id == user_id) return connection;
    }

    Network_Node node = { 0 };
    if(!connect_to_user_server(user_id, &node, US_CLIENT_RS)) return NULL;

    RS_User_Server_Connection new_connection = {0};
    new_connection.user_id = user_id;
    new_connection.node = node;
    return array_add(server->user_server_connections, new_connection);
}


// TODO @Norelease: Make this real.
// NOTE: Return value is true if the user server voted to commit.
// TODO: @Norelease: When we fail here, we just return false. We should not. We should retry until we succeed.
//                   After a number of failed retries, send a report about that.
//                   If the room server can't talk to the user server, there is no idea
//                   that it keeps running anyway.. (????)
//       If the return value is false, either the transaction was aborted because of an abort vote,
//          or there was a communication error.
bool user_server_transaction_prepare(User_ID user, US_Transaction transaction, Room_Server *server, UCB_Packet_Header *_response_header)
{
    //@Norelease: If we fail, disconnect from user server and try to reconnect.
    //           Keep trying until we can connect.
    //           We should not keep trying to connect to the same actual server,
    //           but do the user-id to server lookup every time.

    RS_User_Server_Connection *us_con = find_or_add_connection_to_user_server(user, server);
    if(us_con == NULL) return false;

    Send_Now(USB_TRANSACTION_MESSAGE, &us_con->node, TRANSACTION_PREPARE, &transaction);
    
    if(!expect_type_of_next_ucb_packet(UCB_TRANSACTION_MESSAGE, &us_con->node, _response_header)) return false;
    Assert(_response_header->type == UCB_TRANSACTION_MESSAGE);
    
    return true;
}

bool inbound_item_transaction_prepare(User_ID user, Item_ID item_id, Room_Server *server, Item *_item)
{
    US_Transaction transaction = {0};
    transaction.type = US_T_ITEM;
    transaction.item_details.is_server_bound = false;
    transaction.item_details.client_bound.item_id = item_id;

    UCB_Packet_Header response_header;
    Fail_If_True(!user_server_transaction_prepare(user, transaction, server, &response_header));
    
    Assert(response_header.type == UCB_TRANSACTION_MESSAGE);

    if(response_header.transaction_message.message == TRANSACTION_VOTE_COMMIT) {
        *_item = response_header.transaction_message.commit_vote_payload.item;
        return true;
    }

    Assert(response_header.transaction_message.message == TRANSACTION_VOTE_ABORT);

    return false;
}


bool outbound_item_transaction_prepare(User_ID user, Item *item, Room_Server *server)
{
    US_Transaction transaction = {0};
    transaction.type = US_T_ITEM;
    transaction.item_details.is_server_bound = true;
    transaction.item_details.server_bound.item = *item;

    UCB_Packet_Header response_header;
    Fail_If_True(!user_server_transaction_prepare(user, transaction, server, &response_header));
    
    Assert(response_header.type == UCB_TRANSACTION_MESSAGE);

    if(response_header.transaction_message.message == TRANSACTION_VOTE_COMMIT) {
        return true;
    }

    Assert(response_header.transaction_message.message == TRANSACTION_VOTE_ABORT);

    return false;
}

// TODO @Norelease: Make this real.
// NOTE: Return value is true if there were no com errors.
// TODO: @Norelease: When we fail here, we just return false. We should not. We should retry until we succeed.
//                   After a number of failed retries, send a report about that.
//                   If the room server can't talk to the user server, there is no idea
//                   that it keeps running anyway.. (????)
bool item_transaction_send_decision(bool commit, User_ID user_id, Room_Server *server)
{
    //@Norelease: If we fail, disconnect from user server and try to reconnect.
    //           Keep trying until we can connect.
    //           We should not keep trying to connect to the same actual server,
    //           but do the user-id to server lookup every time.
    
    RS_User_Server_Connection *us_con = find_or_add_connection_to_user_server(user_id, server);
    if(us_con == NULL) {
        RS_Log("Unable to connect to User Server for User ID = %llu.\n", user_id);
        return false;
    }
    
    auto message = (commit) ? TRANSACTION_COMMAND_COMMIT : TRANSACTION_COMMAND_ABORT;
    Send_Now(USB_TRANSACTION_MESSAGE, &us_con->node, message, NULL);
    return true;
}


Entity *find_entity(Entity_ID id, Room *room)
{
    for(int i = 0; i < room->num_entities; i++) {
        auto *e = &room->entities[i];
        if(e->id == id) {
            return e;
        }
    }

    return NULL;
}

Entity *find_player_entity(User_ID user_id, Room *room)
{
    for(int i = 0; i < room->num_entities; i++) {
        auto *e = &room->entities[i];
        if(e->type != ENTITY_PLAYER) continue;
        if(e->player_e.user_id == user_id) {
            return e;
        }
    }

    return NULL;
}

void do_place_item_entity_at_tp(Item *item, v3 tp, Room *room)
{
    v3 p = item_entity_p_from_tp(tp, item);
                            
    Entity e = {0};
    *static_cast<S__Entity *>(&e) = create_item_entity(item, p, room->t);
    e.id   = 1 + room->next_entity_id_minus_one++;
                        
    room->entities[room->num_entities++] = e;
}

bool pick_up_item_entity(User_ID as_user, Entity *e, Room *room, Room_Server *server)
{
    Assert(as_user != NO_USER);
    Assert(e->type == ENTITY_ITEM);
    
    update_entity_item(e, room->t);
    
    bool can_commit = true;
    
    if(can_commit && !outbound_item_transaction_prepare(as_user, &e->item_e.item, server)) {
        can_commit = false;
    }

    // Check if we can commit //
    if(can_commit) {
        can_commit = true;
    }

    if(can_commit)
    {
        // Do commit //
                        
        if(!item_transaction_send_decision(true, as_user, server)) {
            // @Norelease: Handle com error. This proc should probably not
            //             return on error, but instead retry until it succeeds (See comment for the proc.)
        }
        else {
            // REMOVE ENTITY //
            *e = room->entities[room->num_entities-1];
            room->num_entities--;
            room->did_change = true;
            
            return true;
        }
    }
    else {
        // Do abort //
                        
        if(!item_transaction_send_decision(false, as_user, server)) {
            // @Norelease: Handle com error. This proc should probably not
            //             return on error, but instead retry until it succeeds (See comment for the proc.)
        }
    }

    return false;
}

// NOTE: Returns starting position.
v3 player_walk_to(v3 p1, Entity *e, Room *room)
{
    Assert(e->type == ENTITY_PLAYER);
    auto *player_e = &e->player_e;

    v3 p0 = entity_position(e, room->t);
    
    player_e->walk_t0 = room->t;
    player_e->walk_p0 = p0;
    player_e->walk_p1 = p1;

    room->did_change = true;

    return p0;
}

bool perform_entity_action_if_possible(User_ID as_user, Entity *e, Entity_Action action, Room *room, Room_Server *server)
{
    Entity *player = NULL;
    if(as_user != NO_USER) {
        player = find_player_entity(as_user, room);
    }
    
    if(!entity_action_predicted_possible(action, e, as_user, room->t, NULL)) return false;

    bool action_performed = false;
    
    switch(action.type) {
        case ENTITY_ACT_PICK_UP: {
            Assert(as_user != NO_USER);
            Assert(e->type == ENTITY_ITEM);
            
            action_performed = pick_up_item_entity(as_user, e, room, server);
            if(action_performed) {   
                RS_Log("User %llu picked up entity %llu (Item %llu).\n", as_user, e->id, e->item_e.item.id);
            }
        } break;

        case ENTITY_ACT_HARVEST: {
            Assert(as_user != NO_USER);
            Assert(e->type == ENTITY_ITEM);
            
            // @Temporary @Norelease
            action_performed = pick_up_item_entity(as_user, e, room, server);
            if(action_performed) {   
                RS_Log("User %llu picked up (\"Harvested\") entity %llu (Item %llu).\n", as_user, e->id, e->item_e.item.id);
            }
        } break;

        case ENTITY_ACT_SET_POWER_MODE: {
            Assert(e->type == ENTITY_ITEM);
            Assert(e->item_e.item.type == ITEM_MACHINE);
            
            auto *machine = &e->item_e.machine;
            if(action.set_power_mode.set_to_on) {
                Assert(machine->stop_t >= machine->start_t);
                machine->start_t = room->t;
            } else {
                Assert(machine->start_t > machine->stop_t);
                machine->stop_t = room->t;
            }

            room->did_change = true;
            
            action_performed = true;
        } break;

        default: Assert(false); return false;
    }

    return action_performed;
}

// NOTE: Returns true if the action should be dequeued.
bool update_player_action(Player_Action *action, Entity *player_entity, Room *room, Room_Server *server)
{
    Assert(player_entity->type == ENTITY_PLAYER);
    auto *player_e = &player_entity->player_e;
    
    if(doubles_equal(action->next_update_t, room->t))
    {
        switch(action->type) {
            case PLAYER_ACT_ENTITY: {
                auto *x = &action->entity;

                auto *target = find_entity(x->target, room);
                if (!target) return true; // This is OK, the entity might have been destroyed. (We should probably stop walking then, but keep this check anyway.)
                        
                if(!perform_entity_action_if_possible(player_e->user_id, target, x->action, room, server)) { // NOTE: We might do actions in multiple steps. That's why we have an if statement here.
                    return true;
                }
                else {
                    return true;
                }
            } break;

            case PLAYER_ACT_WALK: {
                return true;
            } break;

            default: Assert(false); return true;
        }
    }

    if(action->next_update_t <= room->t) {
        RS_Log("ERROR: action->next_update_t > room->t | next = %f, room = %f.\n", action->next_update_t, room->t);
        Assert(false);
        return true;
    }
    
    return false;
}

void dequeue_player_action(int index, Entity *e, Room *room, Room_Server *server);

// NOTE: Returns false if the action can't start.
bool begin_performing_first_player_action(Entity *e, Room *room, Room_Server *server)
{
    Assert(e->type == ENTITY_PLAYER);
    auto *player_e = &e->player_e;
    
    Assert(player_e->action_queue_length > 0);
    auto *action = &player_e->action_queue[0];

    double next_update_t = 0;

    switch(action->type) {
        case PLAYER_ACT_WALK: {
            v3 p1 = action->walk.p1;
            v3 p0 = player_walk_to(p1, e, room);
            next_update_t = (room->t + magnitude(p1 - p0) / player_walk_speed);
        } break;
        
        case PLAYER_ACT_ENTITY: {
            auto *x = &action->entity;
            auto *target_entity = find_entity(x->target, room);
            if(!target_entity) return false;

            Assert(player_e->user_id != NO_USER);
            if(!entity_action_predicted_possible(x->action, target_entity, player_e->user_id, room->t, NULL))
                return false;
            
            v3 p1 = entity_position(target_entity, room->t);
            v3 p0 = player_walk_to(p1, e, room);
            next_update_t = (room->t + magnitude(p1 - p0) / player_walk_speed);
        } break;

        default: {
            RS_Log("Unhandled player action type %d in %s.\n", action->type, __FUNCTION__);
            Assert(false);
            return false;
        } break;
    }

    action->next_update_t = next_update_t;

    if(action->next_update_t < room->t) {
        Assert(false);
        return false;
    }
    
    while(doubles_equal(action->next_update_t, room->t))
    {
        if(update_player_action(action, e, room, server)) {
            Assert(player_e->action_queue_length > 0);
            dequeue_player_action(0, e, room, server);
            break;
        }
    
        if(action->next_update_t < room->t) {
            Assert(false);
            return false;
        }
    }

    return true;
}

void dequeue_player_action(int index, Entity *e, Room *room, Room_Server *server)
{
    Assert(e->type == ENTITY_PLAYER);
    auto *player_e = &e->player_e;
    
    Assert(index >= 0);
    Assert(index < player_e->action_queue_length);

    for(int i = index; i < player_e->action_queue_length-1; i++)
        player_e->action_queue[i] = player_e->action_queue[i+1];
    player_e->action_queue_length--;

    if(index == 0 && player_e->action_queue_length > 0)
    {
        // If we dequeued the first action, we need to start the new first one.
        if(!begin_performing_first_player_action(e, room, server))
        {
            // Recursive. We want to continue dequeueing until an action is successfully
            // started, or the queue is empty.
            dequeue_player_action(0, e, room, server);
        }
    }
    
    room->did_change = true;
}

bool enqueue_player_action(Entity *e, Player_Action *action, Room *room, Room_Server *server)
{
    Assert(e->type == ENTITY_PLAYER);
    auto *player_e = &e->player_e;

    if(player_e->action_queue_length >= ARRLEN(player_e->action_queue))
        return false;

    bool first_in_queue = (player_e->action_queue_length == 0);

    player_e->action_queue[player_e->action_queue_length++] = *action;
    room->did_change = true;

    if(first_in_queue) {
        if(!begin_performing_first_player_action(e, room, server)) {
            RS_Log("Failed to start performing the action we just enqueued....\n");
            Assert(false);
            dequeue_player_action(0, e, room, server);
            return false;
        }
    }

    
    return true;
}


void update_entity(Entity *e, Room *room, Room_Server *server, bool *_do_destroy)
{
    *_do_destroy = false;

    switch(e->type) {
        case ENTITY_ITEM: {
            
            if(e->item_e.item.type != ITEM_MACHINE) return;

            auto *machine = &e->item_e.machine;
            if(machine->stop_t >= machine->start_t) return; // Machine is not running.

            double time_since_start = room->t - machine->start_t;
            if(doubles_equal(time_since_start, 3.0 /* @Robustness: Define this somewhere */))
            {
                Item plant = {0};
                plant.id = random_int(200, 300); // @Norelease: Make unique
                plant.type = ITEM_PLANT;

                v3 tp = entity_position(e, room->t) - V3_Y * 2;
        
                if(can_place_item_entity_at_tp(&plant, tp, room->t, room, room->entities, room->num_entities))
                {
                    do_place_item_entity_at_tp(&plant, tp, room);
                }
                machine->stop_t = room->t;
                room->did_change = true;
            }
        } break;

        case ENTITY_PLAYER: {
            auto *player_e = &e->player_e;

            if(player_e->action_queue_length == 0) break;
            auto *action = &player_e->action_queue[0];

            if(update_player_action(action, e, room, server)) {
                dequeue_player_action(0, e, room, server);
            }
        } break;
    }
}



double max_update_step_delta_time_for_entity(Entity *e, Room *room)
{
    switch(e->type) {
        case ENTITY_ITEM: {
            
            if(e->item_e.item.type != ITEM_MACHINE) break;

            auto *machine = &e->item_e.machine;
            if(machine->stop_t >= machine->start_t) break; // Machine is not running.

            double time_since_start = room->t - machine->start_t;
            if(time_since_start >= 3.0) break;

            return 3.0 - time_since_start;
            
        } break;

        case ENTITY_PLAYER: {
            auto *player_e = &e->player_e;

            if(player_e->action_queue_length == 0) break;
            auto *action = &player_e->action_queue[0];

            if(action->next_update_t <= room->t) {
                // The action should have been removed or it should have updated its next_update_t.
                Assert(false);
                break;
            }

            return action->next_update_t - room->t;
            
        } break;

        default: break;
    }

    return DBL_MAX;
}

double max_update_step_delta_time_for_room(Room *room)
{
    double result = DBL_MAX;
    
    for(int i = 0; i < room->num_entities; i++) {
        auto max_dt = max_update_step_delta_time_for_entity(&room->entities[i], room);
        
        if(max_dt > 0 && !doubles_equal(max_dt, 0)) {
            if(max_dt < result) result = max_dt;
        }
    }

    Assert(result > 0);
    Assert(!doubles_equal(result, 0));
    return result;
}

void update_room(Room *room, int index, Room_Server *server)
{
    Assert(room->t > 0); // Should be initialized when created

    auto t = get_time();

    double last_t = room->t;
    double total_dt = t - last_t;
    double dt_left = total_dt;
    while(dt_left)
    {
        double dt = min(dt_left, max_update_step_delta_time_for_room(room));
        Assert(dt > 0);

        room->t += dt;

        for(int i = 0; i < room->num_entities; i++)
        {
            bool do_destroy;
            update_entity(&room->entities[i], room, server, &do_destroy);
            if(do_destroy) {
                room->entities[i] = room->entities[room->num_entities-1];
                room->num_entities--;
                room->did_change = true;
            }
        }

        dt_left -= dt;
    }

    Assert(floats_equal(room->t, t));
    room->t = t;
}

void add_chat_message(User_ID user, String text, Room *room)
{
    const Allocator_ID allocator = ALLOC_MALLOC;
    
    Chat_Message message = {0};
    message.t    = room->t;
    message.user = user;
    message.text = copy_of(&text, allocator);

    Assert(room->num_chat_messages <= MAX_CHAT_MESSAGES_PER_ROOM);
    if(room->num_chat_messages == MAX_CHAT_MESSAGES_PER_ROOM)
    {
        clear(&room->chat_messages[0].text, allocator);
        
        for(int i = 0; i < room->num_chat_messages-1; i++) {
            room->chat_messages[i] = room->chat_messages[i+1];
        }
        room->num_chat_messages--;
    }

    Assert(room->num_chat_messages >= 0);
    Assert(room->num_chat_messages < MAX_CHAT_MESSAGES_PER_ROOM);

    room->chat_messages[room->num_chat_messages++] = message;
    room->did_change = true;
}

bool read_and_handle_rsb_packet(RS_Client *client, RSB_Packet_Header header, Room *room, Room_Server *server)
{
    // NOTE: RSB_HELLO and RSB_GOODBYE is handled somewhere else.

    auto *node = &client->node;
    
    switch(header.type) {
        case RSB_CLICK_TILE: {
            TIMED_BLOCK("RSB_CLICK_TILE");
            
            auto &p = header.click_tile;

            Fail_If_True(p.tile_ix >= room_size_x * room_size_y);
            Fail_If_True(p.tile_ix < 0);

            v3 tp = tp_from_index(p.tile_ix);

            if(client->user != NO_USER)
            {   
                if(p.item_to_place != NO_ITEM)
                {            
                    // IMPORTANT: If we fail to do the transaction, we still want to send a ROOM_UPDATE.
                    //            This is so the game client can know when to for example remove preview entities.
                    // (@Hack)
                    room->did_change = true;

                    if(room->num_entities < MAX_ENTITIES_PER_ROOM)
                    {
                        bool can_commit = true;

                        // Get item, ask User Server if it can commit //
                        Item item;
                        if(can_commit && !inbound_item_transaction_prepare(client->user, p.item_to_place, server, &item)) {
                            can_commit = false;
                        }

                        // Check if we can commit //
                        if(can_commit) {
                            can_commit = can_place_item_entity_at_tp(&item, tp, room->t, room, room->entities, room->num_entities);
                        }

                        if(can_commit)
                        {
                            // Do commit //
                        
                            if(!item_transaction_send_decision(true, client->user, server)) {
                                // @Norelease: Handle com error. This proc should probably not
                                //             return on error, but instead retry until it succeeds (See comment for the proc.)
                            }

                            do_place_item_entity_at_tp(&item, tp, room);
                        }
                        else {
                            // Do abort //
                        
                            if(!item_transaction_send_decision(false, client->user, server)) {
                                // @Norelease: Handle com error. This proc should probably not
                                //             return on error, but instead retry until it succeeds (See comment for the proc.)
                            }
                        }
                    
                    }
                }
                else {
                
                    Entity *e = find_player_entity(client->user, room);
                    if(e) {
                        auto p1 = tp_from_index(p.tile_ix);
                    
                        Player_Action action = {0};
                        action.type = PLAYER_ACT_WALK;
                        action.walk.p1 = p1;

                        if(enqueue_player_action(e, &action, room, server)) {

                        }
                    }
                }
            }            
        } break;

        case RSB_ENTITY_ACTION: {
            auto *p = &header.entity_action;

            Entity *e = find_entity(p->entity, room);
            if(!e) {
                return true; // It's OK, the entity might have been destroyed after the client sent the request.
            }

            Entity *player = NULL;
            if(client->user != NO_USER) {
                player = find_player_entity(client->user, room);
                if(!player) {
                    RS_Log("Player trying to perform entity action, but that player's entity was not found.\n");
                    return false;
                }
            }

            if(player) {
                auto *player_e = &player->player_e;

                Player_Action player_action = {0};
                player_action.type = PLAYER_ACT_ENTITY;
                player_action.entity.target = e->id;
                player_action.entity.action = p->action;

                if(enqueue_player_action(player, &player_action, room, server)) {
                    
                }
            }
            else {
                Assert(client->user == NO_USER);
                if(perform_entity_action_if_possible(client->user, e, p->action, room, server)) {

                }
            }
            
        } break;


        case RSB_CHAT: {
            auto *p = &header.chat;
            
            Fail_If_True(client->user == NO_USER);
            
            add_chat_message(client->user, p->message_text, room);
        } break;
            
        default: {
            RS_Log("Client (socket = %lld) sent invalid RSB packet type (%u).\n", client->node.socket.handle, header.type);
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

                int num_received_packets = 0;
                
                while(num_received_packets < MAX_INBOUND_RS_CLIENT_PACKETS_PER_LOOP) {

                    bool error;
                    if(!receive_next_network_node_packet(&client->node, &error)) {
                        if(error) {
                            RS_Log("Failed to receive_next_network_node_packet from client. Disconnecting client.\n");
                            disconnect_room_client(client); // @Cleanup @Boilerplate
                            c--;
                        }
                        break;
                    }
                    num_received_packets++;
                    
                    RSB_Packet_Header header;
                    RSB_Header(client, &header);
                    if(client == NULL) { c--; break; }

                    bool do_disconnect = false;

                    if(header.type == RSB_GOODBYE) {
                        // TODO @Norelease: Better logging, with more client info etc.
                        RS_Log("Client (socket = %lld) sent goodbye message.\n", client->node.socket.handle);
                        do_disconnect = true;
                    }
                    else if(!read_and_handle_rsb_packet(client, header, room, server)) {
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
            update_room(room, i, server);
            //

            // SEND UPDATES TO CLIENTS //
            if(!room->did_change) continue;
            room->did_change = false;
            
            // @Speed: Loop over just the sockets, not the whole Client structs.
            // @Speed: Prepare the data to send (all packets combined, endiannessed etc) and then just write that to all the clients.

            // TODO @Norelease: Only send tiles/entities that changed.
            for(int c = 0; c < clients.n; c++) {
                auto *client = &clients[c];

                int first_chat_ix = room->num_chat_messages;
                for(int i = 0; i < room->num_chat_messages; i++)
                {
                    auto *chat = &room->chat_messages[i];
                    if (chat->t >= client->room_t_on_connect) {
                        first_chat_ix = i;
                        break;
                    }
                }
                int num_chat_messages = room->num_chat_messages - first_chat_ix;
                Assert(num_chat_messages >= 0);
                
                RCB_Packet(client, ROOM_UPDATE, 0, room_size, room->tiles, room->num_entities, room->entities,
                           num_chat_messages, room->chat_messages + first_chat_ix);
                if(client == NULL) c--;
            }
        }

        add_new_room_clients(server);
            
        platform_sleep_milliseconds(1);
    }

    RS_Log("Stopping listening loop...");
    stop_listening_loop(listening_loop, &listening_loop_thread);
    RS_Log_No_T("Done.\n");

    // TODO @Incomplete: Disconnect all clients here, after exiting listening loop

    RS_Log("Exiting.\n");

    return 0;
}
