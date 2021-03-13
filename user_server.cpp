
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


// NOTE: There is one version of this for the user server, and one for the room server.
Item_ID reserve_item_id(User_Server *server)
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
Item create_item(Item_Type_ID type, User_ID owner, User_Server *server)
{
    Item item = {0};
    
    item.id    = reserve_item_id(server);
    item.type  = type;
    item.owner = owner;
    
    return item;
}


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
    
    Array<US_Client, ALLOC_MALLOC> empty_client_array = {0};

    User_ID next_user_id = 1;
    
    for(int i = 0; i < ARRLEN(usernames); i++) {
        User user = {0};
        user.id       = next_user_id++; 
        user.username = copy_cstring_to_string(usernames[i], allocator);
        user.color    = { random_float(),  random_float(),  random_float(), 1 };
        user.money    = random_int(-100, 1000);

        Assert(user.id != NO_USER);

        for(int j = 0; j < ARRLEN(user.inventory); j++)
        {
            auto type = (Item_Type_ID)random_int(0, ITEM_NONE_OR_NUM);

            Inventory_Slot slot = {0};
            if(type != ITEM_NONE_OR_NUM) {
                slot.flags |= INV_SLOT_FILLED;
                slot.item   = create_item(type, user.id, server);

                if((item_types[slot.item.type].container_form == FORM_LIQUID) &&
                   slot.item.type != ITEM_BLENDER)
                {
                    float capacity = liquid_container_capacity(&slot.item);

                    if(random_float() > .5f)
                        slot.item.liquid_container.liquid.type = (Liquid_Type)random_int(0, LQ_NONE_OR_NUM);
                    else
                        slot.item.liquid_container.liquid.type = LQ_WATER;
                    
                    if(slot.item.liquid_container.liquid.type != LQ_NONE_OR_NUM)
                    {
                        float r = random_float();
                        if(r < .25) {
                            slot.item.liquid_container.amount = 0;
                        } else if (r > .75) {
                            slot.item.liquid_container.amount = capacity;
                        } else {
                            slot.item.liquid_container.amount = floorf((0.5f + 0.5f * random_float()) * capacity);
                        }
                    }
                }
            }
            
            user.inventory[j] = slot;
        }
        
        array_add(server->users,   user);
        array_add(server->clients, empty_client_array);
    }

    US_Log("Dummy users:\n");
    for(int i = 0; i < server->users.n; i++) {
        auto &u = server->users[i];
        auto &c = u.color;

        int num_spaces = max(1, 32 - u.username.length);
        US_Log("    %.*s", (int)u.username.length, u.username.data);
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
    while(listening_loop_running(loop, true, &client_accepted, &client_socket, LOG_TAG_US_LIST))
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
        if(hello->client_type != US_CLIENT_PLAYER) {
            client.server_id = hello->non_player.server_id;
        }

        US_LIST_Log("Requested user: %llu\n", client.user_id);

        enqueue_client(&client, queue);

        platform_sleep_milliseconds(1);
    }

    US_LIST_Log("Exiting.");
    return 0;
}

bool unreserve_money(User *user, u32 server_id, Money amount)
{
    for (int k = 0; k < user->money_reservations.n; k++) 
    {
        auto *reservation = &user->money_reservations[k];

        if (reservation->server_id != server_id) continue;

        Assert(reservation->amount >= amount);
        reservation->amount -= amount;

        Assert(reservation->amount >= 0);
        if (reservation->amount <= 0) {
            array_unordered_remove(user->money_reservations, k);
        }

        user->total_money_reserved -= amount;

        return true;
    }

    return false;
}


void commit_transaction(US_Transaction t, User *user, u32 server_id)
{
    Assert(server_id > 0);
    
    for(int i = 0; i < t.num_operations; i++)
    {
        Assert(i < ARRLEN(t.operations));
        auto *op = &t.operations[i];

        switch(op->type)  // @Jai: #complete
        {
            case US_T_ITEM_TRANSFER: {
                auto *x = &op->item_transfer;
            
                if(x->is_server_bound)
                {
                    auto *sb = &x->server_bound;
                    
                    Inventory_Slot *slot = NULL;
                    if(sb->slot_ix_plus_one > 0) {
                        Assert(sb->slot_ix_plus_one > 0 && sb->slot_ix_plus_one <= ARRLEN(user->inventory));
                        slot = &user->inventory[sb->slot_ix_plus_one-1];
                    } else {
                        slot = find_first_empty_inventory_slot(user);
                    }

                    if(slot) {

                        auto slot_ix = (slot - user->inventory);
                    
                        Assert(!(slot->flags & INV_SLOT_FILLED));
                        Assert(!(slot->flags & INV_SLOT_RESERVED) || user->inventory_reservations[slot_ix] == server_id);
                    
                        slot->item = sb->item;
                        slot->flags |= INV_SLOT_FILLED;
                    
                        user->did_change = true;
                    } else {
                        Assert(false); // TODO @ReportError @Norelease
                    }
                }
                else
                {
                    auto *cb = &x->client_bound;
                    
                    bool done = false;
                
                    for(int i = 0; i < ARRLEN(user->inventory); i++) {

                        auto *slot = &user->inventory[i];
                        if(!(slot->flags & INV_SLOT_FILLED)) continue;

                        if(slot->item.id == cb->item_id)
                        {
                            Assert(!(slot->flags & INV_SLOT_RESERVED) || user->inventory_reservations[i] == server_id);

                            slot->flags &= ~(INV_SLOT_FILLED | INV_SLOT_RESERVED);
                            user->inventory_reservations[i] = 0;
                        
                            user->did_change = true;

                            done = true;
                            break;
                        }
                    }
                    Assert(done); // TODO @ReportError @Norelease
                }
            
            } break;

            case US_T_ITEM_RESERVE: {
                auto *x = &op->item_reserve;

                bool done = false;
                
                for(int i = 0; i < ARRLEN(user->inventory); i++) {

                    auto *slot = &user->inventory[i];
                    if(!(slot->flags & INV_SLOT_FILLED)) continue;

                    if(slot->item.id == x->item_id)
                    {
                        Assert(!(slot->flags & INV_SLOT_RESERVED));

                        slot->flags |= INV_SLOT_RESERVED;
                        
                        Assert(user->inventory_reservations[i] == 0);
                        user->inventory_reservations[i] = server_id;
                        
                        user->did_change = true;

                        done = true;
                        break;
                    }
                }
                Assert(done); // TODO @ReportError @Norelease
                
            } break;

            case US_T_ITEM_UNRESERVE: {
                auto *x = &op->item_unreserve;

                bool done = false;
                
                for(int i = 0; i < ARRLEN(user->inventory); i++) {

                    auto *slot = &user->inventory[i];
                    if(!(slot->flags & INV_SLOT_FILLED)) continue;

                    if(slot->item.id == x->item_id)
                    {
                        Assert((slot->flags & INV_SLOT_RESERVED));

                        slot->flags &= ~(INV_SLOT_RESERVED);
                        
                        Assert(user->inventory_reservations[i] != 0);
                        user->inventory_reservations[i] = 0;
                        
                        user->did_change = true;

                        done = true;
                        break;
                    }
                }
                Assert(done); // TODO @ReportError @Norelease

            } break;

            case US_T_SLOT_RESERVE: {
                auto *x = &op->slot_reserve;
                
                bool done = false;

                auto *slot = find_first_empty_inventory_slot(user);
                if(slot) {
                    Assert(!(slot->flags & INV_SLOT_FILLED));
                    Assert(!(slot->flags & INV_SLOT_RESERVED));
                    
                    slot->flags |= INV_SLOT_RESERVED;

                    auto slot_ix = (slot - user->inventory);
                    Assert(slot_ix >= 0);
                    
                    Assert(user->inventory_reservations[slot_ix] == 0);
                    user->inventory_reservations[slot_ix] = server_id;
                    
                    user->did_change = true;

                    done = true;
                    break;
                }
                Assert(done); // TODO @ReportError @Norelease
                
            } break;

            case US_T_SLOT_UNRESERVE: {
                auto *x = &op->slot_unreserve;

                Assert(x->slot_ix >= 0 && x->slot_ix < ARRLEN(user->inventory));

                auto *slot = &user->inventory[x->slot_ix];

                Assert(slot->flags & INV_SLOT_RESERVED);
                slot->flags &= ~(INV_SLOT_RESERVED);
                
                auto slot_ix = (slot - user->inventory);
                Assert(slot_ix >= 0);

                Assert(user->inventory_reservations[slot_ix] == server_id);
                user->inventory_reservations[slot_ix] = 0;
                
                user->did_change = true;
                
            } break;

                
            case US_T_MONEY_RESERVE: {
                auto *x = &op->money_reserve;

                Assert(x->amount > 0);
                Assert(user->total_money_reserved <= user->money - x->amount);

                bool found = false;
                for(int k = 0; k < user->money_reservations.n; k++) {
                    auto *reservation = &user->money_reservations[k];
                    if(reservation->server_id != server_id) continue;

                    reservation->amount += x->amount;
                    found = true;
                    break;
                }

                if(!found) {
                    Money_Reservation reservation = {0};
                    reservation.server_id = server_id;
                    reservation.amount = x->amount;
                    array_add(user->money_reservations, reservation);
                }
                
                user->total_money_reserved += x->amount;

                
            } break;

            case US_T_MONEY_UNRESERVE: {
                auto *x = &op->money_unreserve;

                Assert(user->total_money_reserved >= x->amount);

                bool done = unreserve_money(user, server_id, x->amount);
                Assert(done);
               
            } break;

            case US_T_MONEY_TRANSFER: {
                auto *x = &op->money_transfer;
                
                if (x->do_unreserve) {
                    Assert(x->amount < 0);
                    bool unreserve_success = unreserve_money(user, server_id, -x->amount);
                    Assert(unreserve_success);
                }

                Assert(user->money + x->amount - user->total_money_reserved >= 0);

                user->money += x->amount;
                
            } break;


            default: Assert(false); return;
        }    
    }
    
}

bool transaction_possible(US_Transaction t, User *user, u32 server_id, UCB_Transaction_Commit_Vote_Payload *_commit_vote_payload)
{
    Assert(server_id > 0);
    
    Zero(*_commit_vote_payload);

    _commit_vote_payload->num_operations = t.num_operations;

    for(int i = 0; i < t.num_operations; i++)
    {
        Assert(i < ARRLEN(t.operations));

        auto *op = &t.operations[i];
        auto *op_payload = &_commit_vote_payload->operation_payloads[i];

        _commit_vote_payload->operation_types[i] = op->type;

        switch(op->type) {
            case US_T_ITEM_TRANSFER: {
                auto *x = &op->item_transfer;
            
                if(x->is_server_bound)
                {
                    auto *sb = &x->server_bound;

                    Inventory_Slot *slot = NULL;
                    if(sb->slot_ix_plus_one > 0) {
                        Assert(sb->slot_ix_plus_one > 0 && sb->slot_ix_plus_one <= ARRLEN(user->inventory));
                        slot = &user->inventory[sb->slot_ix_plus_one-1];
                    } else {
                        slot = find_first_empty_inventory_slot(user);
                    }

                    if(!slot) return false;
                    
                    auto slot_ix = (slot - user->inventory);
                    Assert(slot_ix >= 0 && slot_ix < ARRLEN(user->inventory));
                    
                    if(slot->flags & INV_SLOT_FILLED)
                        return false;
                    
                    if((slot->flags & INV_SLOT_RESERVED) && user->inventory_reservations[slot_ix] != server_id)
                        return false;
                    
                }
                else
                {
                    auto *cb = &x->client_bound;

                    bool possible = false;
                
                    for(int s = 0; s < ARRLEN(user->inventory); s++) {

                        auto *slot = &user->inventory[s];
                        if(!(slot->flags & INV_SLOT_FILLED)) continue;
                    
                        if(slot->item.id == cb->item_id)
                        {
                            if ((slot->flags & INV_SLOT_RESERVED) &&
                                user->inventory_reservations[s] != server_id)
                            {
                                possible = false;
                                break;
                            }
                        
                            op_payload->item_transfer.item = slot->item;
                            possible = true;

                            break;
                        }
                    }

                    if (!possible) return false;
                }
            
            } break;

            case US_T_ITEM_RESERVE: {
                auto *x = &op->item_reserve;

                bool possible = false;
                
                for(int s = 0; s < ARRLEN(user->inventory); s++) {
                    auto *slot = &user->inventory[s];
                    if(!(slot->flags & INV_SLOT_FILLED)) continue;

                    if(slot->item.id == x->item_id)
                    {
                        if(!(slot->flags & INV_SLOT_RESERVED))
                        {   
                            op_payload->item_reserve.item = slot->item;
                            possible = true;
                        }
                        
                        break;
                    }
                }

                if(!possible) return false;
                
            } break;

            case US_T_ITEM_UNRESERVE: {
                auto *x = &op->item_unreserve;

                bool possible = false;

                for(int s = 0; s < ARRLEN(user->inventory); s++) {
                    auto *slot = &user->inventory[s];
                    if(!(slot->flags & INV_SLOT_FILLED)) continue;

                    if(slot->item.id == x->item_id)
                    {
                        if(slot->flags & INV_SLOT_RESERVED) {
                            Assert(user->inventory_reservations[s] != 0);
                            possible = (user->inventory_reservations[s] == server_id);
                        }
                        
                        break;
                    }
                }

                if(!possible) return false;
                
            } break;

            case US_T_SLOT_RESERVE: {
                auto *x = &op->slot_reserve;
                
                auto *slot = find_first_empty_inventory_slot(user);
                if(!slot) return false;

                op_payload->slot_reserve.slot_ix = (slot - user->inventory);
                
            } break;

            case US_T_SLOT_UNRESERVE: {
                auto *x = &op->slot_unreserve;

                Assert(x->slot_ix >= 0 && x->slot_ix < ARRLEN(user->inventory));
                auto *slot = &user->inventory[x->slot_ix];

                if(!(slot->flags & INV_SLOT_RESERVED)) return false;

                Assert(user->inventory_reservations[x->slot_ix] != 0);
                if(user->inventory_reservations[x->slot_ix] != server_id) return false;
                
            } break;

            case US_T_MONEY_RESERVE: {
                auto *x = &op->money_reserve;
                
                Assert(x->amount > 0);

                Assert(user->total_money_reserved <= user->money);
                if(user->money - user->total_money_reserved < x->amount) return false;
                
            } break;

            case US_T_MONEY_UNRESERVE: {
                auto *x = &op->money_unreserve;

                bool possible = false;
                for(int k = 0; k < user->money_reservations.n; k++) {
                    auto *reservation = &user->money_reservations[k];

                    if(reservation->server_id != server_id) continue;

                    if(reservation->amount >= x->amount) {
                        possible = true;
                        break;
                    }
                }

                if(!possible) return false;
                
            } break;

            case US_T_MONEY_TRANSFER: {
                auto *x = &op->money_transfer;
                // NOTE: x->amount can be negative.

                Money money_reserved_by_this_client = 0;

                if (x->do_unreserve)
                {
                    if(x->amount >= 0) return false; // Can only unreserve if amount is negative.
                    
                    for (int k = 0; k < user->money_reservations.n; k++) {
                        auto *reservation = &user->money_reservations[k];

                        if (reservation->server_id != server_id) continue;

                        Assert(reservation->amount > 0);
                        money_reserved_by_this_client += reservation->amount;
                    }
                    
                    if(money_reserved_by_this_client < -x->amount) return false; // Not enough reserved to unreserve.
                }

                if(user->money + x->amount - (user->total_money_reserved - money_reserved_by_this_client) < 0) return false;
                
            } break;

            default: Assert(false); return false;
        }
    }

    return true;
}

bool read_and_handle_usb_packet(US_Client *client, USB_Packet_Header header, User *user)
{
    // NOTE: USB_GOODBYE is handled somewhere else.

    auto *node = &client->node;

    if(client->current_transaction_exists) {
        
        Assert(client->type != US_CLIENT_PLAYER);
        
        if(header.type == USB_TRANSACTION_MESSAGE) {
            
            auto &p = header.transaction_message;

            switch(p.message) {
                case TRANSACTION_COMMAND_COMMIT: {
                    commit_transaction(client->current_transaction, user, client->server_id);
                } break;

                case TRANSACTION_COMMAND_ABORT: {
                    // IMPORTANT: If we fail to do the transaction, we still want to send a USER_UPDATE.
                    //            This is so the game client knows when to for example start showing the inventory item
                    //            again that the player was trying to place.
                    // (@Hack)
                    user->did_change = true;
                } break;

                default: Assert(false); return false;
            }
        }
        else {
            US_Log("Was waiting for a USB_TRANSACTION_MESSAGE but got header.type = %u.\n", header.type);
            return false;
        }

        client->current_transaction_exists = false;
        return true;
    }
    
    switch(header.type) {

        case USB_TRANSACTION_MESSAGE: {
            if(client->type == US_CLIENT_PLAYER) {
                US_Log("Player client (socket = %lld) sent USB_TRANSACTION_MESSAGE (Illegal).\n", client->node.socket.handle);
                return false;
            }
            
            auto &p = header.transaction_message;
            
            switch(p.message) {

                case TRANSACTION_PREPARE: {
                    
                    client->current_transaction_exists = true;
                    client->current_transaction        = p.transaction;

                    UCB_Transaction_Commit_Vote_Payload commit_vote_payload;
                    if(transaction_possible(p.transaction, user, client->server_id, &commit_vote_payload))
                    {
                        Send_Now(UCB_TRANSACTION_MESSAGE, &client->node,
                                 TRANSACTION_VOTE_COMMIT, &commit_vote_payload);
                    }
                    else
                    {
                        Send_Now(UCB_TRANSACTION_MESSAGE, &client->node,
                                 TRANSACTION_VOTE_ABORT);
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

    // REMEMBER: This is special, so don't do anything fancy here that can put us in an infinite disconnection loop.
    if(!send_UCB_GOODBYE_packet_now(&client->node)) {
        US_Log("Failed to write UCB Goodbye.\n");
    }
    
    if(!platform_close_socket(&client->node.socket)) {
        US_Log("Failed to close user client socket.\n");
    }
    
    bool found_user = false;
    
    auto *server = client->server;
    for(int i = 0; i < server->users.n; i++)
    {
        if(server->users[i].id == client->user_id) {
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
                if(server->users[u].id == requested_user) {
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

            String socket_str = socket_to_string(client->node.socket);
            US_Log("User (username = %.*s): Added client (socket = %.*s).\n", (int)user->username.length, user->username.data, (int)socket_str.length, socket_str.data);

            if(client->type == US_CLIENT_PLAYER)
            {
                UCB_Packet(USER_INIT, client, user->id, user->username, user->color, user->money, user->total_money_reserved, user->inventory);
                if(!client) {
                    US_Log("Unable to init user client.\n"); // initialize_user_client() disconnects client for us on error.
                }
            }
        }

        queue->num_clients = 0;
    }
    unlock_mutex(queue->mutex);
}

bool get_next_us_client_packet_or_disconnect(US_Client *client, USB_Packet_Header *_header, bool *_client_disconnected)
{
    *_client_disconnected = false;
    
    return true;
}


// REMEMBER to init_user_server before starting this.
//          IMPORTANT: Do NOT deinit_user_server after
//                     this is done -- this proc will do that for you.
DWORD user_server_main_loop(void *server_) {

    const Allocator_ID allocator = ALLOC_MALLOC;
    
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
                    if(client == NULL) {
                        c--;
                        break;
                    }

                    bool do_disconnect = false;
                    
                    if(header.type == USB_GOODBYE) {
                        // TODO @Norelease: Better logging, with more client info etc.
                        US_Log("Client (socket = %lld) sent goodbye message.\n", client->node.socket.handle);
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

                if(user->did_change) {
                    UCB_Packet(USER_UPDATE, client, user->id, user->username, user->color, user->money, user->total_money_reserved, user->inventory);
                }
            }

            user->did_change = false;
        }
        

        add_new_user_clients(server);
        platform_sleep_milliseconds(1);
    }

    US_Log("Stopping listening loop...");
    stop_listening_loop(listening_loop, &listening_loop_thread);
    US_Log_No_T("Done.\n");
    
    US_Log("Exiting.\n");

    return 0;
}
