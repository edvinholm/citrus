
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


// NOTE: There is one version of this for the user server, and one for the room server.
Item_ID reserve_item_id(Room_Server *server)
{
    u32 server_id = server->server_id;
    u32 internal_origin = 0;
    u64 number = server->next_item_number++;

    u64 origin = server_id;
    origin <<= 32;
    origin |= internal_origin;

    return { origin, number };
}

// NOTE: There is one version of this for the user server, and one for the room server.
Item create_item(Item_Type_ID type, User_ID owner, Room_Server *server)
{
    Item item = {0};
    
    item.id    = reserve_item_id(server);
    item.type  = type;
    item.owner = owner;
    
    return item;
}

void create_dummy_entities(Room *room, Room_Server *server)
{   
    v3 pp = { room_size_x/2 + 8, room_size_y/2 };
    for(int i = 0; i < 2; i++)
    {
        Item_Type_ID item_type_id = (i == 0) ? ITEM_CHESS_BOARD : ITEM_CHAIR;
        Item_Type *item_type = item_types + item_type_id;
        
        Item item = {0};
        item.id = reserve_item_id(server);
        item.type = item_type_id; 

        Entity e = {0};
        *static_cast<S__Entity *>(&e) = create_item_entity(&item, pp, room->t);
        e.id = 1 + room->next_entity_id_minus_one++;

        Assert(room->num_entities < ARRLEN(room->entities));
        room->entities[room->num_entities++] = { e };

        pp.x += item_type->volume.x + 4;
    }
}

void create_dummy_rooms(Room_Server *server)
{
    double t = get_time();

    Array<RS_Client, ALLOC_MALLOC> empty_client_array = {0};

    const int map_pixel_components = 3;
    
    int map_sx, map_sy, unused;
    auto *map = stbi_load("res/maps/default_01.bmp", &map_sx, &map_sy, &unused, 3);
    defer(free(map););

    Assert(map_sx == room_size_x && map_sy == room_size_y);
    
    for(int i = 0; i < 8; i++) {
        Room room = {0};
        room.t = get_time();
        room.randomize_cooldown = random_float() * 3.0;

        create_dummy_entities(&room, server);

#if 1
        
        auto *map_at = map;
        for(int t = 0; t < room_size_x * room_size_y; t++) {
            auto *tile = room.tiles + t;

            static_assert(map_pixel_components == 3);
            auto r = *map_at++;
            auto g = *map_at++;
            auto b = *map_at++;

            if(r == 255 && g == 255 && b == 255) {
                *tile = TILE_WALL;
            } else if(r == 128 && g == 128 && b == 128) {
                // Sidewalk (@Norelease)
                *tile = TILE_SAND;
            } else if(r == 0 && g == 128 && b == 0) {
                *tile = TILE_GRASS;
            } else if(r == 128 && g == 64 && b == 0) {
                // Wooden floor or something. (@Norelease)
                *tile = TILE_SAND;
            }
        }
        
#elif 0
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
            auto wsa_error_1 = WSAGetLastError();                       \
                                                                        \
            String socket_str = socket_to_string(client->node.socket);       \
                                                                        \
            RS_Log("Client disconnected from room %d due to RCB packet failure (socket = %.*s), E1: %d).\n", \
                   RS_Client_Ptr->room, (int)socket_str.length, socket_str.data, wsa_error_1); \
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
    RCB_Packet(client, ROOM_INIT, room->t, room->num_entities, room->entities, room->tiles, &room->walk_map);
    return true;
}


void player_set_walk_path(v3 *path, u16 length, Entity *e, Room *room);

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
            
            String socket_str = socket_to_string(client->node.socket);
            RS_Log("Room %d: Added client (socket = %.*s)\n", client->room, (int)socket_str.length, socket_str.data);

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
                
                v3s p;
                do    p = { random_int(1, room_size_x-1), random_int(1, room_size_y-1), 0 };
                while(room->walk_map.nodes[p.y * room_size_x + p.x].flags & UNWALKABLE);

                auto *player_e = &e.player_e;
                player_e->user_id     = client->user;

                v3 path[] = { V3(p), V3(p) };
                player_set_walk_path(path, 2, &e, room);
                
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
    const Allocator_ID allocator = ALLOC_MALLOC;
    
    for(int i = 0; i < server->user_server_connections.n; i++)
    {
        auto *connection = &server->user_server_connections[i];
        if(connection->user_id == user_id) return connection;
    }

    Network_Node node = { 0 };
    if(!connect_to_user_server(user_id, &node, US_CLIENT_RS, server->server_id)) return NULL;

    RS_User_Server_Connection new_connection = {0};
    new_connection.user_id = user_id;
    new_connection.node = node;
    return array_add(server->user_server_connections, new_connection);
}



// TODO @Norelease: Make this real.
// TODO: @Norelease: When we fail here (due to com errors), we just return false. We should not. We should retry until we succeed.
//                   After a number of failed retries, send a report about that.
//                   If the room server can't talk to the user server, there is no idea
//                   that it keeps running anyway.. (????)
bool rs_us_inbound_item_transaction_prepare(User_ID user, Item_ID item_id, Room_Server *server, Item *_item)
{
    // CREATE THE TRANSACTION //
    US_Transaction_Operation operation = {0};
    operation.type = US_T_ITEM_TRANSFER;
    operation.item_transfer.is_server_bound = false;
    operation.item_transfer.client_bound.item_id = item_id;
    
    US_Transaction transaction = {0};
    transaction.num_operations = 1;
    transaction.operations[0] = operation;
    // ////////////////////// //

    // @Norelease: If we fail, disconnect from user server and try to reconnect.
    //             Keep trying until we can connect.
    //             We should not keep trying to connect to the same actual server,
    //             but do the user-id to server lookup every time.

    RS_User_Server_Connection *us_con = find_or_add_connection_to_user_server(user, server);
    if(us_con == NULL) return false;
    
    UCB_Packet_Header response_header;
    Fail_If_True(!user_server_transaction_prepare(transaction, &us_con->node, &response_header));
    
    Assert(response_header.type == UCB_TRANSACTION_MESSAGE);

    auto *t_message = &response_header.transaction_message;
    if(t_message->message == TRANSACTION_VOTE_COMMIT)
    {
        auto *cvp = &response_header.transaction_message.commit_vote_payload;
        
        Fail_If_True(cvp->num_operations != 1);
        *_item = cvp->operation_payloads[0].item_transfer.item;
        
        return true;
    }

    Assert(response_header.transaction_message.message == TRANSACTION_VOTE_ABORT);

    return false;
}


// TODO @Norelease: Make this real.
// TODO: @Norelease: When we fail here (due to com errors), we just return false. We should not. We should retry until we succeed.
//                   After a number of failed retries, send a report about that.
//                   If the room server can't talk to the user server, there is no idea
//                   that it keeps running anyway.. (????)
bool rs_us_outbound_item_transaction_prepare(User_ID user, Item *item, Room_Server *server)
{
    // CREATE THE TRANSACTION //
    US_Transaction_Operation operation = {0};
    operation.type = US_T_ITEM_TRANSFER;
    operation.item_transfer.is_server_bound = true;
    operation.item_transfer.server_bound.item = *item;
    
    US_Transaction transaction = {0};
    transaction.num_operations = 1;
    transaction.operations[0] = operation;
    // ////////////////////// //

    // @Norelease: If we fail, disconnect from user server and try to reconnect.
    //             Keep trying until we can connect.
    //             We should not keep trying to connect to the same actual server,
    //             but do the user-id to server lookup every time.

    RS_User_Server_Connection *us_con = find_or_add_connection_to_user_server(user, server);
    if(us_con == NULL) return false;
    
    UCB_Packet_Header response_header;
    Fail_If_True(!user_server_transaction_prepare(transaction, &us_con->node, &response_header));
    
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
bool rs_us_transaction_send_decision(bool commit, User_ID user_id, Room_Server *server)
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

    Fail_If_True(!user_server_transaction_send_decision(commit, &us_con->node));
                 
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


// @Speed @Speed @Speed @Norelease
// IMPORTANT: There is one implementation for this for the client, and one for the room server.
//            Because C++ sucks. @Jai
Entity *item_entity_of_type_at(Item_Type_ID type, v3 p, double world_t, Room *room)
{
    for(int i = 0; i < room->num_entities; i++) {
        auto *e = room->entities + i;
        if(e->type != ENTITY_ITEM) continue;
        if(e->item_e.item.type != type) continue;

        if(entity_position(e, world_t, room) == p) {
            return e;
        }
    }
    return NULL;
}


void dequeue_player_action(int index, Entity *e, Room *room, Room_Server *server);


void player_set_walk_path(v3 *path, u16 length, Entity *e, Room *room)
{
    const auto allocator = ALLOC_MALLOC;
    
    Assert(e->type == ENTITY_PLAYER);
    auto *player_e = &e->player_e;

    if(player_e->walk_path != NULL)
        dealloc(player_e->walk_path, allocator);

    // @Speed: Reuse memory!
    size_t size = sizeof(*path) * length;
    player_e->walk_path = (v3 *)alloc(size, allocator);
    
    player_e->walk_t0 = room->t;
    player_e->walk_path_length = length;
    memcpy(player_e->walk_path, path, size);
    
    room->did_change = true;
}

void player_stop_walking(Entity *e, Room *room)
{
    v3 p      = compfloor(entity_position(e, room->t, room));
    v3 path[] = { p, p };
    player_set_walk_path(path, ARRLEN(path), e, room);
}



bool player_walk_to(v3 p1, Entity *e, Room *room, double *_dur = NULL, v3 *_p0 = NULL, bool allow_same_start_and_end_tile = true, bool *_was_same_start_and_end_tile = NULL)
{
    if (!allow_same_start_and_end_tile) {
        Assert(_was_same_start_and_end_tile);
        *_was_same_start_and_end_tile = false;
    }
    
    // @Volatile @Boilerplate: The other player_walk_to() 
    v3 p0 = entity_position(e, room->t, room);
    v3s start_tile = { (s32)roundf(p0.x), (s32)roundf(p0.y), (s32)roundf(p0.z) };
    v3s end_tile   = { (s32)roundf(p1.x), (s32)roundf(p1.y), (s32)roundf(p1.z) };
    
    if(_p0)  *_p0 = p0;
    
    if(start_tile == end_tile)
    {
        if(_dur) *_dur = 0;

        if (!allow_same_start_and_end_tile) {
            *_was_same_start_and_end_tile = true;
            return false;
        }

        player_stop_walking(e, room);
        
        return true;
    }

    Array<v3, ALLOC_TMP> path = {0};
    if(!find_path_to_any(p0, &p1, 1, &room->walk_map, true, &path, _dur)) return false; // @Cleanup: Do we really need to call xx_to_any here?

    Assert(path.n >= 2);
    
    player_set_walk_path(path.e, path.n, e, room);
    return true;
}

void update_walk_map_and_paths(Room *room, Room_Server *server)
{
    // UPDATE WALK MAP //
    generate_walk_map(room, player_entity_volume, &room->walk_map);

    // UPDATE WALK PATHS //
    for(int i = 0; i < room->num_entities; i++) {

        auto *e = room->entities + i;
        if(e->type != ENTITY_PLAYER) continue;

        auto *player_e = &e->player_e;
        Assert(player_e->walk_path_length >= 2);

        // NOTE: If the walk path visits the same tile multiple times, this would not always work.
        v3 p1 = player_e->walk_path[player_e->walk_path_length-1];
        
        double dur;
        v3 p0; // @Unused
        bool was_same_start_and_end_tile;
        if(player_walk_to(p1, e, room, &dur, &p0, false, &was_same_start_and_end_tile))
        {
            Assert(dur > 0);

            // @Norelease.    
            // @Hack: We don't know if the walk path has to do with the current action.
            if (player_e->action_queue_length > 0)
                player_e->action_queue[0].next_update_t = room->t + dur;
        }
        else
        {
            // NOTE: If was_same_start_and_end_tile == true, that means (if there are no portals)
            //       that we are already at the target position, and we don't need to update the path.
            //       IMPORTANT: We are not allowed to set a zero-duration path here anyway, because
            //                  that would result in a zero update step.
            if (!was_same_start_and_end_tile)
            {
                // @Norelease.
                // @Hack: We don't know if the walk path has to do with the current action.
                //        We also don't know if the action can be cancelled at this time.
                if (player_e->action_queue_length > 0)
                    dequeue_player_action(0, e, room, server);
            }
        }

        room->did_change = true;
    }
}


void do_place_item_entity_at_tp(Item *item, v3 tp, Room *room, Room_Server *server)
{
    v3 p = item_entity_p_from_tp(tp, item);
                            
    Entity e = {0};
    *static_cast<S__Entity *>(&e) = create_item_entity(item, p, room->t);
    e.id   = 1 + room->next_entity_id_minus_one++;
                        
    room->entities[room->num_entities++] = e;

    update_walk_map_and_paths(room, server);
}

bool place_item_entity_at_tp_if_possible(Item *item, v3 tp, Room *room, Room_Server *server)
{
    if(can_place_item_entity_at_tp(item, tp, room->t, room->entities, room->num_entities, room))
    {
        do_place_item_entity_at_tp(item, tp, room, server);
        return true;
    }
    return false;
}

void set_held(Entity *e, Entity *holder, bool should_be_held)
{
    if(should_be_held) {
        Assert(e->held_by   == NO_ENTITY);
        Assert(holder->holding == NO_ENTITY);
    
        holder->holding = e->id;
        e->held_by = holder->id;
    }
    else {
        Assert(e->held_by      == holder->id);
        Assert(holder->holding == e->id);
        
        holder->holding = NO_ENTITY;
        e->held_by      = NO_ENTITY;
    }
}


void destroy_entity(Entity *e, Room *room)
{
    Assert(room->num_entities > 0);
    
    Assert(e >= room->entities);
    Assert(e <= room->entities + room->num_entities-1);
    Assert((u64)((u8 *)e - (u8 *)room->entities) % sizeof(Entity) == 0);

    if(e->held_by != NO_ENTITY) {
        auto *holder = find_entity(e->held_by, room);
        if(holder) set_held(e, holder, false);
    }

    if(e->holding != NO_ENTITY) {
        auto *held = find_entity(e->holding, room);
        if(held) set_held(held, e, false);
    }
    
    *e = room->entities[room->num_entities-1];
    room->num_entities--;
    
    room->did_change = true;
}


bool place_item_entity_in_inventory(User_ID as_user, Entity *e, Room *room, Room_Server *server)
{
    Assert(as_user != NO_USER);
    Assert(e->type == ENTITY_ITEM);
    
    update_entity_item(e, room->t);
    
    bool can_commit = true;
    
    if(can_commit && !rs_us_outbound_item_transaction_prepare(as_user, &e->item_e.item, server)) {
        can_commit = false;
    }

    // Check if we can commit //
    if(can_commit) {
        can_commit = true;
    }

    if(can_commit)
    {
        // Do commit //
                        
        if(!rs_us_transaction_send_decision(true, as_user, server)) {
            // @Norelease: Handle com error. This proc should probably not
            //             return on error, but instead retry until it succeeds (See comment for the proc.)
        }
        else {
            destroy_entity(e, room);
            return true;
        }
    }
    else {
        // Do abort //
                        
        if(!rs_us_transaction_send_decision(false, as_user, server)) {
            // @Norelease: Handle com error. This proc should probably not
            //             return on error, but instead retry until it succeeds (See comment for the proc.)
        }
    }

    return false;
}



void pick_up_item_entity(Entity *item_entity, Entity *player_entity, Room *room, Room_Server *server)
{
    Assert(item_entity->type   == ENTITY_ITEM);
    
    Assert(player_entity->type == ENTITY_PLAYER);
    auto player_e = &player_entity->player_e;
    
    update_entity_item(item_entity, room->t);

    Assert(item_entity->held_by == NO_ENTITY);
    Assert(player_entity->holding == NO_ENTITY); // This should have been checked before calling this procedure. For example in entity_action_predicted_possible... -EH, 2021-02-11

    
    set_held(item_entity, player_entity, true);

    
    update_walk_map_and_paths(room, server);            
    room->did_change = true;        
}


bool perform_player_action_if_possible(Player_Action *action, User_ID as_user, Room *room, Room_Server *server)
{
    Assert(as_user != NO_USER);
    
    Entity      *player = find_player_entity(as_user, room);
    if(player == NULL) return false;
    Assert(player->type == ENTITY_PLAYER);

    Player_State player_state = player_state_of(player, room->t, room);
    if(!player_action_predicted_possible(action, &player_state, room->t, room)) return false;
    
    switch(action->type) { // @Jai: #complete
        
        case PLAYER_ACT_ENTITY: {
            auto *entity_action = &action->entity.action;

            auto *target = find_entity(action->entity.target, room);
            if (!target) return true; // This is OK, the entity might have been destroyed. (We should probably stop walking then, but keep this check anyway.)
    
            switch(entity_action->type) { // @Jai: #complete

                case ENTITY_ACT_PICK_UP: {
                    Assert(as_user != NO_USER);
                    Assert(target->type == ENTITY_ITEM);

                    Assert(player);
            
                    pick_up_item_entity(target, player, room, server);
                    RS_Log("User %llu picked up entity %llu (Item %llu:%llu).\n", as_user, target->id, target->item_e.item.id.origin, target->item_e.item.id.number); // @Jai: Print function for Item_ID struct.
                    return true;
            
                } break;
        
                case ENTITY_ACT_PLACE_IN_INVENTORY: {
                    Assert(as_user != NO_USER);
                    Assert(target->type == ENTITY_ITEM);
            
                    bool action_performed = place_item_entity_in_inventory(as_user, target, room, server);
                    // REMEMBER: e is invalid after this.
            
                    if(action_performed) {   
                        RS_Log("User %llu placed item %llu:%llu (entity %llu) in inventory.\n", as_user, target->item_e.item.id.origin, target->item_e.item.id.number, target->id); // @Jai: Print function for Item_ID struct.
                        return true;
                    }
                    return false;
                } break;

                case ENTITY_ACT_HARVEST: {
                    Assert(as_user != NO_USER);
                    Assert(target->type == ENTITY_ITEM);
            
                    // @Temporary @Norelease
                    bool action_performed = place_item_entity_in_inventory(as_user, target, room, server);
                    // REMEMBER: e is invalid after this.
            
                    if(action_performed) {   
                        RS_Log("User %llu picked up (\"Harvested\") entity %llu (Item %llu:%llu).\n", as_user, target->id, target->item_e.item.id.origin, target->item_e.item.id.number); // @Jai: Print function for Item_ID struct.
                        return true;
                    }
                    return false;
                } break;
            
                case ENTITY_ACT_WATER: {
                    Assert(as_user != NO_USER);
                    Assert(target->type == ENTITY_ITEM);
                    auto *item_e = &target->item_e;

                    Assert(target->item_e.item.type == ITEM_PLANT);
                    auto *plant_e = &target->item_e.plant;

                    Assert(player);
                    Assert(player->holding != NO_ENTITY);
            
                    // @Cleanup: Can we do a proc that finds an entity and check that it is an expected item type?
                    auto *watering_can = find_entity(player->holding, room);
                    Assert(watering_can);
                    Assert(watering_can->type == ENTITY_ITEM);
                    Assert(watering_can->item_e.item.type == ITEM_WATERING_CAN);

                    auto *water_level = &watering_can->item_e.item.watering_can.water_level;
                    if(*water_level >= 0.25f) // @Norelease @Volatile: define constant somewhere. We have it in entity_action_predicted_possible and perform_entity_action_if_possible.
                    {
                        plant_e->grow_progress_on_plant += 0.05f;
                        *water_level -= 0.25f;

                        room->did_change = true;
                        return true;
                    }
                    return false;
            
                } break;

                case ENTITY_ACT_SET_POWER_MODE: {
                    Assert(target->type == ENTITY_ITEM);
            
                    Assert(target->item_e.item.type == ITEM_MACHINE);
                    auto *machine = &target->item_e.machine;
            
                    if(entity_action->set_power_mode.set_to_on) {
                        Assert(machine->stop_t >= machine->start_t);
                        machine->start_t = room->t;
                    } else {
                        Assert(machine->start_t > machine->stop_t);
                        machine->stop_t = room->t;
                    }

                    room->did_change = true;
                    return true;
                } break;

                case ENTITY_ACT_CHESS: {
                    auto *chess_action = &entity_action->chess;
            
                    Assert(target->type == ENTITY_ITEM);
                    Assert(target->item_e.item.type == ITEM_CHESS_BOARD);
                    auto *board = &target->item_e.chess_board;

                    if(perform_chess_action_if_possible(chess_action, player_state.user_id, board)) {    
                        room->did_change = true;
                        return true;
                    }
                    return false;
                } break;

                case ENTITY_ACT_SIT_OR_UNSIT: {
                    auto *sit = &entity_action->sit_or_unsit;
                    
                    Assert(target->type == ENTITY_ITEM);

                    Assert(target->item_e.item.type == ITEM_CHAIR);

                    if(sit->unsit) {
                        Assert(player->player_e.sitting_on == target->id);
                        player->player_e.sitting_on = NO_ENTITY;
                    } else {
                        Assert(player->player_e.sitting_on != target->id);
                        player->player_e.sitting_on = target->id;
                    }

                    room->did_change = true;                    
                    return true;

                } break;

                default: Assert(false); return false;
            }
    
        } break;

        case PLAYER_ACT_WALK: {
            return true;
        } break;

        case PLAYER_ACT_PUT_DOWN: {
            auto *x = &action->put_down;
            
            auto *held = find_entity(player->holding, room);
            if(!held) {
                Assert(false); // This should not happen. Holding relations should be removed when the held entity is removed.
                player->holding = NO_ENTITY;
                return true;
            }
            
            Assert(held->held_by == player->id);
            Assert(held->type == ENTITY_ITEM);
            
            v3 p = item_entity_p_from_tp(x->tp, &held->item_e.item);
            Assert(item_entity_can_be_at(held, p, room->t, room->entities, room->num_entities, room));

            //--
            
            held->item_e.p = p;
            set_held(held, player, false);
            
            update_walk_map_and_paths(room, server);
                
            return true;
        } break;

        default: Assert(false); return true;
    }

    return false;
}

// NOTE: Returns true if the action should be dequeued.
bool update_player_action(Player_Action *action, Entity *player, Room *room, Room_Server *server)
{
    Assert(player->type == ENTITY_PLAYER);
    auto *player_e = &player->player_e;
    
    if(doubles_equal(action->next_update_t, room->t))
    {
        if(!perform_player_action_if_possible(action, player_e->user_id, room, server)) { // NOTE: I put an 'if' here to remind us that perform_player_action_if_possible() returns a bool.
            return true;
        } else {
            return true;
        }
    }

    if(action->next_update_t <= room->t) {
        RS_Log("ERROR: action->next_update_t > room->t | next = %f, room = %f.\n", action->next_update_t, room->t);
        Assert(false);
        return true;
    }
    
    return false;
}

// NOTE: Returns false if the action can't start.
bool begin_performing_first_player_action(Entity *e, Room *room, Room_Server *server)
{
    Assert(e->type == ENTITY_PLAYER);
    auto *player_e = &e->player_e;
    
    Assert(player_e->action_queue_length > 0);
    auto *action = &player_e->action_queue[0];

    double next_update_t = 0;

    Array<v3, ALLOC_TMP> path = {0};
    double path_duration;

    Player_State player_state = player_state_of(e, room->t, room);
    if (!player_action_predicted_possible(action, &player_state, room->t, room, NULL, &path, &path_duration)) return false;

    if (path.n == 0) player_stop_walking(e, room);
    else             player_set_walk_path(path.e, path.n, e, room);
    action->next_update_t = room->t + path_duration; // NOTE: path_duration can be zero here.

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

// NOTE: Will apply the action to player_state.
bool enqueue_player_action_(Entity *e, Player_Action *action, Player_State *player_state, Room *room, Room_Server *server, int depth = 0)
{
    Assert(e->type == ENTITY_PLAYER);
    auto *player_e = &e->player_e;
    
    Optional<Player_Action> action_needed_before = {0};
    while(!player_action_predicted_possible(action, player_state, room->t, room, &action_needed_before))
    {
        Player_Action act;
        if(!get(action_needed_before, &act)) return false;
        
        if(!enqueue_player_action_(e, &act, player_state, room, server, depth + 1)) return false;    
    }
    
    if(player_e->action_queue_length >= ARRLEN(player_e->action_queue) - depth)
        return false;

    bool first_in_queue = (player_e->action_queue_length == 0);

    player_e->action_queue[player_e->action_queue_length++] = *action;
    room->did_change = true;

    if(first_in_queue) {
        if(!begin_performing_first_player_action(e, room, server)) {
            dequeue_player_action(0, e, room, server);
            return false;
        }
    }

    apply_actions_to_player_state(player_state, action, 1, room->t, room, NULL);

    return true;
}

bool enqueue_player_action(Entity *e, Player_Action *action, Room *room, Room_Server *server)
{
    Assert(e->type == ENTITY_PLAYER);
    auto *player_e = &e->player_e;
    
    // @Speed: Cache state_after_completed_action_queue as we do on the client. -EH, 2021-02-26
    Player_State player_state = player_state_of(e, room->t, room);
    apply_actions_to_player_state(&player_state, player_e->action_queue, player_e->action_queue_length, room->t, room, NULL);
    
    return enqueue_player_action_(e, action, &player_state, room, server);
}


void update_entity(Entity *e, Room *room, Room_Server *server, bool *_do_destroy)
{
    *_do_destroy = false;

    switch(e->type) {
        case ENTITY_ITEM: {

            auto *item = &e->item_e.item;
            if(item->type != ITEM_MACHINE) return;

            auto *machine = &e->item_e.machine;
            if(machine->stop_t >= machine->start_t) return; // Machine is not running.

            double time_since_start = room->t - machine->start_t;
            if(doubles_equal(time_since_start, 3.0 /* @Robustness: Define this somewhere */))
            {
                Item plant = create_item(ITEM_PLANT, item->owner, server);

                v3 tp = entity_position(e, room->t, room) - V3_Y * 2;

                place_item_entity_at_tp_if_possible(&plant, tp, room, server);
                
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

#if DEBUG
    static double last_result = 898989.454545;
#endif
    
    for(int i = 0; i < room->num_entities; i++) {
        auto max_dt = max_update_step_delta_time_for_entity(&room->entities[i], room);
        
        if(max_dt > 0 && !doubles_equal(max_dt, 0)) {
            if(max_dt < result) result = max_dt;
        }
    }

    Assert(result > 0);
    Assert(!doubles_equal(result, 0));

#if DEBUG
    last_result = result;
#endif

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
            auto *e = &room->entities[i];
            
            bool do_destroy;
            update_entity(e, room, server, &do_destroy);
            if(do_destroy) {
                destroy_entity(e, room);
                i--;
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
                        if(can_commit && !rs_us_inbound_item_transaction_prepare(client->user, p.item_to_place, server, &item)) {
                            can_commit = false;
                        }

                        // Check if we can commit //
                        if(can_commit) {
                            can_commit = can_place_item_entity_at_tp(&item, tp, room->t, room->entities, room->num_entities, room);
                        }

                        if(can_commit)
                        {
                            // Do commit //
                        
                            if(!rs_us_transaction_send_decision(true, client->user, server)) {
                                // @Norelease: Handle com error. This proc should probably not
                                //             return on error, but instead retry until it succeeds (See comment for the proc.)
                            }

                            do_place_item_entity_at_tp(&item, tp, room, server);
                        }
                        else {
                            // Do abort //
                        
                            if(!rs_us_transaction_send_decision(false, client->user, server)) {
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

                        if(p.default_action_is_put_down)
                        {   
                            Player_Action action = {0};
                            action.type = PLAYER_ACT_PUT_DOWN;
                            action.put_down.tp = p1;

                            if(enqueue_player_action(e, &action, room, server)) {

                            }
                        }
                        else
                        {
                            Player_Action action = {0};
                            action.type = PLAYER_ACT_WALK;
                            action.walk.p1 = p1;

                            if(enqueue_player_action(e, &action, room, server)) {

                            }
                        }
                    }
                }
            }            
        } break;

        case RSB_PLAYER_ACTION: {
            auto *player_action = &header.player_action;

            Entity *player = NULL;
            if(client->user == NO_USER) return false;
            
            player = find_player_entity(client->user, room);
            if(!player) {
                RS_Log("Player trying to perform an action, but that player's entity was not found.\n");
                return false;
            }

            Assert(player);
            
            auto *player_e = &player->player_e;
            
            if(enqueue_player_action(player, player_action, room, server)) {
                
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

    // INIT ROOMS //
    for(int i = 0; i < server->rooms.n; i++)
    {
        update_walk_map_and_paths(&server->rooms[i], server);
    }
    

    // START LISTENING LOOP //
    Thread listening_loop_thread;
    if(!start_room_server_listening_loop(server, &listening_loop_thread)) {
        RS_Log("Failed to start listening loop.");
        return 1; // TODO @Norelease: Main server program must know about this!
    }
    // 
           
    double t = get_time();

    Array<int, ALLOC_MALLOC> clients_to_disconnect = {0};
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
                
                RCB_Packet(client, ROOM_UPDATE, 0, room_size, room->tiles, &room->walk_map, room->num_entities, room->entities,
                           num_chat_messages, room->chat_messages + first_chat_ix);
                if(client == NULL) c--;
            }
        }

        add_new_room_clients(server);
            
        platform_sleep_milliseconds(1);

        // @Cleanup @Robustness: Disable profiling on server.
        //                       Some procedures that are shared between client and server
        //                       might add profiler nodes. If we never call next_profiler_frame(),
        //                       we will overflow the frame's node array.
        //                       But we can't call next_profiler_frame() from all threads, because
        //                       the profiler is not thread safe!
        next_profiler_frame(PROFILER);
    }

    RS_Log("Stopping listening loop...");
    stop_listening_loop(listening_loop, &listening_loop_thread);
    RS_Log_No_T("Done.\n");

    // TODO @Incomplete: Disconnect all clients here, after exiting listening loop

    RS_Log("Exiting.\n");

    return 0;
}
    
