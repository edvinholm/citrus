
const char *LOG_TAG_MS = "$MS$"; // Market Server
const char *LOG_TAG_MS_LIST = "$MS$LIST$"; // Listening loop

#define MS_Log_T(Tag, ...) \
    printf("[%s]", LOG_TAG_MS);                            \
    Log_T(Tag, __VA_ARGS__)
#define MS_Log(...)                     \
    Log_T(LOG_TAG_MS, __VA_ARGS__)
#define MS_Log_No_T(...)                     \
    Log(__VA_ARGS__)


#define MS_LIST_Log_T(Tag, ...) \
    printf("[%s]", LOG_TAG_MS_LIST);      \
    Log_T(Tag, __VA_ARGS__)
#define MS_LIST_Log(...)                     \
    Log_T(LOG_TAG_MS_LIST, __VA_ARGS__)
#define MS_LIST_No_T(...)                     \
    Log(__VA_ARGS__)


void enqueue_client(MS_Client *client, MS_Client_Queue *queue)
{
    lock_mutex(queue->mutex);
    {
        client->id = queue->next_client_id++;
        
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


bool receive_next_msb_packet(Network_Node *node, MSB_Packet_Header *_packet_header, bool *_error, bool block = false)
{
    if(!receive_next_network_node_packet(node, _error, block)) return false;
    
    *_error = true;
    Read_To_Ptr(MSB_Packet_Header, _packet_header, node);
    *_error = false;
    
    return true;
}

bool expect_type_of_next_msb_packet(MSB_Packet_Type expected_type, Network_Node *node, MSB_Packet_Header *_packet_header)
{
    bool error;
    if(!receive_next_msb_packet(node, _packet_header, &error, true)) return false;
    if(_packet_header->type != expected_type) return false;
    return true;
}

// TODO @Norelease @Robustness @Speed: This thing should have enough information to dismiss clients with invalid user IDs or auth tokens.
DWORD market_server_listening_loop(void *market_server_)
{
    auto *server = (Market_Server *)market_server_;
    auto *queue  = &server->client_queue;

    Listening_Loop *loop = &server->listening_loop;

    MS_LIST_Log("Running.\n");
    
    bool   client_accepted;
    Socket client_socket;
    while(listening_loop_running(loop, true, &client_accepted, &client_socket, LOG_TAG_MS_LIST))
    {
        if(!client_accepted) continue;

        MS_LIST_Log("Client accepted :)\n");

        bool success = true;
        
        if(success && !platform_set_socket_read_timeout(&client_socket, 1000)) {
            MS_LIST_Log("Unable to set read timeout for new client's socket (Last WSA Error: %d).\n", WSAGetLastError());
            success = false;
        }

        Network_Node new_node = {0};
        reset_network_node(&new_node, client_socket);

        MSB_Packet_Header header;
        if(success && !expect_type_of_next_msb_packet(MSB_HELLO, &new_node, &header)) {
            MS_LIST_Log("First packet did not have the expected type MSB_HELLO.\n");
            success = false;
        }

        if(!success)
        {
           if(!platform_close_socket(&client_socket)) {
               MS_LIST_Log("Unable to close new client's socket.\n");
           } else {
               MS_LIST_Log("New client's socket closed successfully.\n");
           }
           continue;
        }

        auto *hello = &header.hello;
        
        // ADD CLIENT TO QUEUE //
        MS_Client client = {0};
        // NOTE: ID is set in enqueue_client  -EH, 2021-02-08.
        client.type     = hello->client_type;
        client.user_id  = hello->user;
        client.server   = server;
        client.node     = new_node;

        MS_LIST_Log("Requested user: %llu\n", client.user_id);

        enqueue_client(&client, queue);

        platform_sleep_milliseconds(1);
    }

    MS_LIST_Log("Exiting.");
    return 0;
}

inline
bool start_market_server_listening_loop(Market_Server *server, Thread *_thread)
{
    return start_listening_loop(&server->listening_loop, MARKET_SERVER_PORT, &market_server_listening_loop, server, _thread, LOG_TAG_MS_LIST);
}


void remove_view_connections(MS_Client *client)
{
    auto *server = client->server;
    
    if(client->view_target.type == MARKET_VIEW_TARGET_ARTICLE &&
       client->view_target.article.article != ITEM_NONE_OR_NUM)
    {
        auto *article = &server->articles[client->view_target.article.article];
        ensure_not_in_array(article->watchers, client->id);
    }
}

void add_view_connections(MS_Client *client)
{
    auto *server = client->server;

    if(client->view_target.type == MARKET_VIEW_TARGET_ARTICLE &&
       client->view_target.article.article != ITEM_NONE_OR_NUM)
    {
        auto *article = &server->articles[client->view_target.article.article];
        ensure_in_array(article->watchers, client->id);
    }
}

void set_view_target(MS_Client *client, Market_View_Target target)
{
    auto *server = client->server;

    // Remove old connections
    remove_view_connections(client);

    // Set
    client->view_target = target;

    // Add new connections;
    add_view_connections(client);
}

void disconnect_market_client(MS_Client *client)
{
    auto client_copy = *client;

    // REMEMBER: This is special, so don't do anything fancy here that can put us in an infinite disconnection loop.
    if(!send_MCB_GOODBYE_packet_now(&client->node)) {
        MS_Log("Failed to write MCB Goodbye.\n");
    }
    
    if(!platform_close_socket(&client->node.socket)) {
        MS_Log("Failed to close market client socket.\n");
    }
    
    auto *server = client->server;

    remove_view_connections(client);

    // Remove client
    auto client_index = client - server->clients.e;
    array_ordered_remove(server->clients, client_index);

    // IMPORTANT that we clear the copy here. Otherwise we would clear the client that is placed where our client was in the array.
    clear(&client_copy);
}



// Sends an MCB packet if MS_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// MS_Client_Ptr is set to NULL.
#define MCB_Packet(Packet_Ident, MS_Client_Ptr, ...) \
    if(MS_Client_Ptr) {                                                 \
        bool success = true;                                            \
        if(success && !enqueue_MCB_##Packet_Ident##_packet(&MS_Client_Ptr->node, __VA_ARGS__)) success = false; \
        Assert(MS_Client_Ptr->node.packet_queue.n == 1);                \
        if(success && !send_outbound_packets(&MS_Client_Ptr->node)) success = false; \
        if(!success) {                                                  \
            MS_Log("Client disconnected from market server due to MCB packet failure (socket = %lld, WSA Error: %d).\n", MS_Client_Ptr->node.socket.handle, WSAGetLastError()); \
            disconnect_market_client(MS_Client_Ptr);                \
            MS_Client_Ptr = NULL;                                       \
        }                                                               \
    }


// Receives an MSB packet if MS_Client_Ptr != NULL.
// If the operation fails, the client is disconnected and
// MS_Client_Ptr is set to NULL.
#define MSB_Header(MS_Client_Ptr, Header_Ptr)           \
    if((MS_Client_Ptr) &&                                             \
       !read_MSB_Packet_Header(Header_Ptr, &MS_Client_Ptr->node)) {    \
        MS_Log("Client disconnected from market server due to MSB header failure (socket = %lld, WSA Error: %d).\n", MS_Client_Ptr->node.socket.handle, WSAGetLastError()); \
        disconnect_market_client(MS_Client_Ptr);                       \
        MS_Client_Ptr = NULL;                                         \
    }                                                               \




void add_new_market_clients(Market_Server *server)
{
    auto *queue = &server->client_queue;
    lock_mutex(queue->mutex);
    {
        Assert(queue->num_clients <= ARRLEN(queue->clients));
        
        for(int c = 0; c < queue->num_clients; c++) {

            MS_Client *client = queue->clients + c;
            User_ID requested_user = client->user_id;

            bool success = true;

            // @Norelease: check that the user exists.
            
            if(success && !send_MCB_HELLO_packet_now(&client->node, MARKET_CONNECT__CONNECTED))
            {
                MS_Log("Failed to write MARKET_CONNECT__CONNECTED to client (socket = %lld).\n", client->node.socket.handle);
                success = false;
            }
            
            if(!success) {
                platform_close_socket(&client->node.socket);
                clear(client);
                continue;
            }


            Market_View_Target view_target = {0};
            view_target.type = MARKET_VIEW_TARGET_ARTICLE;
            view_target.article.article = ITEM_NONE_OR_NUM;
            set_view_target(client, view_target);
                 
            client = array_add(server->clients, *client);

            String socket_str = socket_to_string(client->node.socket);
            MS_Log("Added client (socket = %.*s).\n", (int)socket_str.length, socket_str.data);

            if(client->type == MS_CLIENT_PLAYER)
            {
                MCB_Packet(MARKET_INIT, client);
                if(!client) {
                    MS_Log("Unable to init market client.\n"); // initialize_user_client() disconnects client for us on error.
                }
            }
        }

        queue->num_clients = 0;
    }
    unlock_mutex(queue->mutex);
}

MS_User_Server_Connection *find_or_add_connection_to_user_server(User_ID user_id, Market_Server *server)
{
    const Allocator_ID allocator = ALLOC_MALLOC;

    for(int i = 0; i < server->user_server_connections.n; i++)
    {
        auto *connection = &server->user_server_connections[i];
        if(connection->user_id == user_id) return connection;
    }

    Node_Connection_Arguments args;
    Zero(args);
    args.user.user = user_id;
    args.user.client_type = US_CLIENT_MS;
    args.user.client_node_id = server->server_id;

    Network_Node node = { 0 };
    if(!connect_to_node(&node, NODE_USER, args)) return NULL;

    MS_User_Server_Connection new_connection = {0};
    new_connection.user_id = user_id;
    new_connection.node = node;
    return array_add(server->user_server_connections, new_connection);
}

// TODO @Norelease: Make this real.
// TODO: @Norelease: When we fail here (due to com errors), we just return false. We should not. We should retry until we succeed.
//                   After a number of failed retries, send a report about that.
//                   If the market server can't talk to the user server, there is no idea
//                   that it keeps running anyway.. (????)
bool ms_us_transaction_prepare(User_ID user, US_Transaction transaction, Market_Server *server, UCB_Transaction_Commit_Vote_Payload *_commit_vote_payload)
{
    // @Norelease: If we fail, disconnect from user server and try to reconnect.
    //             Keep trying until we can connect.
    //             We should not keep trying to connect to the same actual server,
    //             but do the user-id to server lookup every time.

    MS_User_Server_Connection *us_con = find_or_add_connection_to_user_server(user, server);
    if(us_con == NULL) return false;

    UCB_Packet_Header response_header;
    Fail_If_True(!user_server_transaction_prepare(transaction, &us_con->node, &response_header));

    Assert(response_header.type == UCB_TRANSACTION_MESSAGE);
    
    auto *t_message = &response_header.transaction_message;
    if(t_message->message == TRANSACTION_VOTE_COMMIT)
    {
        auto *cvp = &response_header.transaction_message.commit_vote_payload;
        
        Fail_If_True(cvp->num_operations != transaction.num_operations);
        *_commit_vote_payload = *cvp;
        
        return true;
    }

    Assert(response_header.transaction_message.message == TRANSACTION_VOTE_ABORT);
    
    return false;
}


// TODO @Norelease: Make this real.
// NOTE: Return value is true if there were no com errors.
// TODO: @Norelease: When we fail here, we just return false. We should not. We should retry until we succeed.
//                   After a number of failed retries, send a report about that.
//                   If the market server can't talk to the user server, there is no idea
//                   that it keeps running anyway.. (????)
bool ms_us_transaction_send_decision(bool commit, User_ID user_id, Market_Server *server)
{
    //@Norelease: If we fail, disconnect from user server and try to reconnect.
    //           Keep trying until we can connect.
    //           We should not keep trying to connect to the same actual server,
    //           but do the user-id to server lookup every time.
    
    MS_User_Server_Connection *us_con = find_or_add_connection_to_user_server(user_id, server);
    if(us_con == NULL) {
        MS_Log("Unable to connect to User Server for User ID = %llu.\n", user_id);
        return false;
    }

    Fail_If_True(!user_server_transaction_send_decision(commit, &us_con->node));
                 
    return true;
}


bool reserve_slot_and_money_for_buy(User_ID user, Money money, Market_Server *server, u32 *_slot_ix)
{
    Assert(user != NO_USER);

    
    US_Transaction transaction = {0};

    // slot reserve
    {
        US_Transaction_Operation op = {0};
        op.type = US_T_SLOT_RESERVE;
        
        transaction.operations[transaction.num_operations++] = op;
    }

    if(money > 0) {
        // money reserve
        {
            US_Transaction_Operation op = {0};
            op.type = US_T_MONEY_RESERVE;
            op.money_reserve.amount = money;
            
            transaction.operations[transaction.num_operations++] = op;
        }
    }
    
    bool can_commit = false;

    UCB_Transaction_Commit_Vote_Payload cvp;
    if(ms_us_transaction_prepare(user, transaction, server, &cvp)) {
        MS_Log("Slot reservation success.\n");
        *_slot_ix = cvp.operation_payloads[0].slot_reserve.slot_ix;
        can_commit = true;
    } else {
        MS_Log("Slot reservation failure.\n");
        can_commit = false;
    }

    // SEND DECISION //
    if(ms_us_transaction_send_decision(can_commit, user, server)) {
        MS_Log("Slot reservation decision send success.\n");
    } else {
        MS_Log("Slot reservation decision send failure. (Should not happen. (?))\n"); // @Norelease
        can_commit = false;
    }
    // //// //////// //

    return can_commit;
}

bool reserve_item_for_sale(User_ID user, Item_ID item_id, Market_Server *server, Item *_item)
{
    Assert(user != NO_USER);
    
    US_Transaction_Operation operation = {0};
    operation.type = US_T_ITEM_RESERVE;
    operation.item_reserve.item_id = item_id;
    
    US_Transaction transaction = {0};
    transaction.num_operations = 1;
    transaction.operations[0] = operation;

    
    bool can_commit = false;

    UCB_Transaction_Commit_Vote_Payload cvp;
    if(ms_us_transaction_prepare(user, transaction, server, &cvp)) {
        MS_Log("Item reservation preparation success.\n");
        *_item = cvp.operation_payloads[0].item_reserve.item;
        can_commit = true;
    } else {
        MS_Log("Item reservation preparation failure. (Should not happen. (?))\n"); // @Norelease
        can_commit = false;
    }

    // SEND DECISION //
    if(ms_us_transaction_send_decision(can_commit, user, server)) {
        MS_Log("Item reservation decision send success.\n");
    } else {
        MS_Log("Item reservation decision send failure.\n");
        can_commit = false;
    }
    // //// //////// //

    return can_commit;
}

bool exchange_item_and_money(User_ID item_receiver, User_ID money_receiver, Item_ID item_id, Money money, u32 item_receiver_slot_ix, Money item_receiver_reserved_money, Market_Server *server)
{
    bool can_commit = true;

    // MONEY RECEIVER TRANSACTION //
    US_Transaction mr_transaction = {0};

    // item transfer
    {
        US_Transaction_Operation op = {0};
        op.type = US_T_ITEM_TRANSFER;
        op.item_transfer.is_server_bound = false;
        op.item_transfer.client_bound.item_id = item_id;

        mr_transaction.operations[mr_transaction.num_operations++] = op;
    }

    // money transfer
    {
        US_Transaction_Operation op = {0};
        op.type = US_T_MONEY_TRANSFER;
        op.money_unreserve.amount = +money;
            
        mr_transaction.operations[mr_transaction.num_operations++] = op;
    }
    // /////////////////////////// //
    
    UCB_Transaction_Commit_Vote_Payload mr_cvp;
    if(can_commit && !ms_us_transaction_prepare(money_receiver, mr_transaction, server, &mr_cvp)) {
        MS_Log("Item <-> Money exchange failed due to transaction prepare failure for MR transaction. (Should not happen, because should have reserved everything)\n"); // @Norelease: IF this happens, @ReportError, unreserve stuff and remove orders.
        can_commit = false;
    }

    Item item = mr_cvp.operation_payloads[0].item_transfer.item;

    if(can_commit) {
        // ITEM RECEIVER TRANSACTION //
        US_Transaction ir_transaction = {0};

        // slot unreserve
        {
            US_Transaction_Operation op = {0};
            op.type = US_T_SLOT_UNRESERVE;
            op.slot_unreserve.slot_ix = item_receiver_slot_ix;
            
            ir_transaction.operations[ir_transaction.num_operations++] = op;
        }

        Assert(item_receiver_reserved_money >= money);
        if(item_receiver_reserved_money - money > 0) {
            // money unreserve
            {
                US_Transaction_Operation op = {0};
                op.type = US_T_MONEY_UNRESERVE;
                op.money_unreserve.amount = item_receiver_reserved_money - money; // The rest of the reserved money will be unreserved by the MONEY_TRANSFER operation.
            
                ir_transaction.operations[ir_transaction.num_operations++] = op;
            }
        }

        // item transfer
        {
            US_Transaction_Operation op = {0};
            op.type = US_T_ITEM_TRANSFER;
            op.item_transfer.is_server_bound = true;
            op.item_transfer.server_bound.item = item;
            op.item_transfer.server_bound.slot_ix_plus_one = item_receiver_slot_ix + 1;

            ir_transaction.operations[ir_transaction.num_operations++] = op;
        }

        // money transfer
        {
            US_Transaction_Operation op = {0};
            op.type = US_T_MONEY_TRANSFER;
            op.money_transfer.amount       = -money;
            op.money_transfer.do_unreserve = (op.money_transfer.amount < 0); // Cannot unreserve 0.
            
            ir_transaction.operations[ir_transaction.num_operations++] = op;
        }
        // /////////////////////////// //

        UCB_Transaction_Commit_Vote_Payload ir_cvp;
        if(can_commit && !ms_us_transaction_prepare(item_receiver, ir_transaction, server, &ir_cvp)) {
            MS_Log("Item <-> Money exchange failed due to transaction prepare failure for IR transaction. (Should not happen, because should have reserved everything)\n"); // @Norelease: IF this happens, @ReportError, unreserve stuff and remove orders.
            can_commit = false;
        }

        if(!ms_us_transaction_send_decision(can_commit, item_receiver, server)) {
            MS_Log("Item <-> Money: Failed to send transaction decision to IR. (This should not happen)\n");
            can_commit = false;
        }
    }
    
    if(!ms_us_transaction_send_decision(can_commit, money_receiver, server)) {
        MS_Log("Item <-> Money: Failed to send transaction decision to MR. (This should not happen)\n");
        can_commit = false;
    }
    // --

    return can_commit;
}

bool send_market_update_to_article_watcher(MS_Client *client, Item_Type_ID article_id, Market_Article *article, Market_Server *server)
{
    Assert(client->view_target.type == MARKET_VIEW_TARGET_ARTICLE &&
           client->view_target.article.article == article_id); // We pass article_id and article to this procedure because of @Speed. But the passed article should be the client's watched article!

    Market_View view;
    Zero(view);
    view.target = client->view_target;

    auto *article_view = &view.article;
    
    article_view->num_prices = 0;
    if(article != NULL)
    {
        article_view->num_prices = article->price_history_length;

        static_assert(sizeof(article->price_history[0]) == sizeof(article_view->prices[0]));
        static_assert(sizeof(article->price_history)    == sizeof(article_view->prices));

        memcpy(article_view->prices, article->price_history, sizeof(article_view->prices));
    }

    Assert(article != NULL || article_view->num_prices == 0);
            
    MCB_Packet(MARKET_UPDATE, client, &view);

    return true;
}


bool send_market_update(MS_Client *client)
{
    Market_Server *server = client->server;
    //--
    
    if(client->view_target.type == MARKET_VIEW_TARGET_ARTICLE)
    {
        auto *article_target = &client->view_target.article;
        
        Market_Article *article = NULL;
        auto article_id = article_target->article;
        if(article_id != ITEM_NONE_OR_NUM)
        {
            Assert(article_id >= 0 &&
                   article_id < ARRLEN(server->articles));
            
            article = &server->articles[article_id];
        }    

        return send_market_update_to_article_watcher(client, article_id, article, server);
    }

    
    Market_View view;
    Zero(view);
    view.target = client->view_target;

    switch(view.target.type) {
        case MARKET_VIEW_TARGET_ARTICLE: Assert(false); return false; // This should be handled above.

        case MARKET_VIEW_TARGET_ORDERS:
        {
            auto *orders_view = &view.orders;
        } break;
    }
    
    MCB_Packet(MARKET_UPDATE, client, &view);

    return true;
}


bool read_and_handle_msb_packet(MS_Client *client, MSB_Packet_Header header)
{
    auto *node = &client->node;

    Fail_If_True(client->user_id == NO_USER); // IMPORTANT: Put this in each packet type that needs a user, if you want to remove it from here.
    
    switch(header.type) {
        case MSB_PLACE_ORDER: {
            auto *p = &header.place_order;

            // @Norelease: Check that it is a legal order.
            
            Market_Order order = {0};
            order.is_buy_order = p->is_buy_order;
            order.price        = p->price;
            order.user         = client->user_id;

            auto *server = client->server;

            if(order.is_buy_order) {
                auto *x = &p->buy;

                Assert(x->item_type >= 0 && x->item_type < ARRLEN(server->articles));
                auto *article = &server->articles[x->item_type];
                
                String item_type_name = item_types[x->item_type].name;
                MS_Log("User %lld wants to buy a %.*s for ¤%lld.\n", client->user_id, (int)item_type_name.length, item_type_name.data, p->price);

                u32 slot_ix;
                if(reserve_slot_and_money_for_buy(order.user, order.price, server, &slot_ix))
                {
                    order.buy.reserved_slot_ix = slot_ix;
                    array_add(article->orders, order);
                } else {
                    MS_Log("Failed to reserve slot and money.\n");
                }
                
            } else {
                auto *x = &p->sell;

                MS_Log("User %lld wants to sell item %lld:%lld for ¤%lld.\n", client->user_id, x->item_id.origin, x->item_id.number, p->price);

                Item item;
                if(reserve_item_for_sale(order.user, x->item_id, server, &item)) {
                    MS_Log("Successfully reserved the item.\n");

                    Assert(item.type >= 0 && item.type < ARRLEN(server->articles));
                    auto *article = &server->articles[item.type];
                    
                    order.sell.item = item;
                    array_add(article->orders, order);
                    
                    MS_Log("Order added to article %d.\n", item.type);
                    
                } else {
                    MS_Log("Failed to reserve the item.\n");
                }
            }
           
        } break;

        case MSB_SET_VIEW_TARGET: {
            auto *p = &header.set_view_target;

            auto *server = client->server;
            
            // @Norelease: Check that article exists.
            set_view_target(client, p->target);
            
            MS_Log("Set view target for client %lld to { .type = %d }\n", client->id, client->view_target.type); // @Jai: Log Market_View_Target struct.

            // SEND MARKET_UPDATE //
            
            Fail_If_True(!send_market_update(client));

            // ////////////////// //
            
        } break;

        default: {
            MS_Log("Client (socket = %lld) sent invalid MSB packet type (%u).\n", client->node.socket.handle, header.type);
            return false;
        } break;
    }

    return true;
}

void add_article_price_to_history(Money price, Market_Article *article)
{
    Assert(article->price_history_length <= ARRLEN(article->price_history));
    
    if(article->price_history_length >= ARRLEN(article->price_history))
    {
        // Remove oldest if array full.
        for(int i = 0; i < ARRLEN(article->price_history)-1; i++) {
            article->price_history[i] = article->price_history[i+1];
        }
        article->price_history_length = ARRLEN(article->price_history)-1;
    }

    article->price_history[article->price_history_length++] = price;
}

void update_article(Market_Article *article, Market_Server *server)
{
    for(int i = 0; i < article->orders.n; i++)
    {
        auto *a = &article->orders[i];
        Assert(a->price >= 0);
        
        Money a_money_delta = a->price;
        if(a->is_buy_order) a_money_delta *= -1;

        Money smallest_combined_money_delta = 1; // Money deltas > 0 means no deal, so setting this to 1 works.
        int b_index = -1;
        
        for(int j = i+1; j < article->orders.n; j++) {
            auto *order = &article->orders[j];

            if(a->is_buy_order == order->is_buy_order) continue;
            if(a->user == order->user) continue;

            Assert(order->price >= 0);
            Money money_delta = order->price;
            if(order->is_buy_order) money_delta *= -1;

            Money combined_money_delta = a_money_delta + money_delta;

            if(combined_money_delta <= 0)
            {
                // It's a match!

                if(combined_money_delta < smallest_combined_money_delta) {
                    // It's the best match yet!
                    smallest_combined_money_delta = combined_money_delta;
                    b_index = j;
                }
            }
        }

        if(b_index >= 0) {
            Assert(b_index > i);
            Assert(b_index < article->orders.n);

            auto *b = &article->orders[b_index];

            Assert(a->is_buy_order != b->is_buy_order);
            Assert(a->user != b->user);
            
            Assert(a->user != NO_USER);
            Assert(b->user != NO_USER);

            auto *buy_order  = (a->is_buy_order) ? a : b;
            auto *sell_order = (a->is_buy_order) ? b : a;

            Money price = min(a->price, b->price);
            
            if(exchange_item_and_money(buy_order->user, sell_order->user, sell_order->sell.item.id, price,
                                       buy_order->buy.reserved_slot_ix, buy_order->price, server))
            {
                MS_Log("Item <-> Money exchange successful. Removing orders and updating article info...\n");

                add_article_price_to_history(price, article);
                
                // Remove b.
                array_ordered_remove(article->orders, b_index);
                // REMEMBER: b_index is invalid now!!

                // Remove a.
                array_ordered_remove(article->orders, i);
                i--;

                article->did_change = true;
            }
        }
    }
}

MS_Client *find_market_client(MS_Client_ID id, Market_Server *server)
{
    for(int i = 0; i < server->clients.n; i++) {
        auto *client = &server->clients[i];
        if(client->id != id) continue;

        return client;
    }

    return NULL;
}

// REMEMBER to init_market_server before starting this.
//          IMPORTANT: Do NOT deinit_market_server after
//                     this is done -- this proc will do that for you.
DWORD market_server_main_loop(void *server_) {

    const Allocator_ID allocator = ALLOC_MALLOC;
    
    Market_Server *server = (Market_Server *)server_;
    defer(deinit_market_server(server););
    MS_Log("Running.\n");

    auto *listening_loop = &server->listening_loop;

    // INITIALIZE MODEL //
    // @Norelease
    for(int i = 0; i < ARRLEN(server->articles); i++) {
        auto *article = &server->articles[i];
        article->price_history_length = random_int(0, ARRLEN(article->price_history));
        Money last = random_int(0, 100);
        for(int i = 0; i < article->price_history_length; i++) {
            auto random = random_int(-10, 10);
            article->price_history[i] = max(0, last + random);
            last = article->price_history[i];
        }
    }

    // START LISTENING LOOP //
    Thread listening_loop_thread;
    if(!start_market_server_listening_loop(server, &listening_loop_thread))
    {
        MS_Log("Failed to start listening loop.");
        return 1; // TODO @Norelease: Main server program must know about this!
    }
    
    while(!get(&server->should_exit)) {

        if(get(&listening_loop->client_accept_failed)) {
            // RESTART LISTENING LOOP //
            MS_Log("Listening loop failed. Joining thread...\n");
            platform_join_thread(listening_loop_thread);
            
            MS_Log("Restarting listening loop...\n");
            deinit_listening_loop(listening_loop);
            if(!start_market_server_listening_loop(server, &listening_loop_thread)) {
                MS_Log("Failed to restart listening loop.\n");
                // TODO @Norelease: Disconnect all clients. (Say goodbye etc)
                // TODO @Norelease: Main server program must know about this!
                return 1;
            }
        }


        // LISTEN TO WHAT THE CLIENTS HAVE TO SAY //
        for(int c = 0; c < server->clients.n; c++) {
            auto *client = &server->clients[c];

            while(true) {
                    
                bool error;
                if(!receive_next_network_node_packet(&client->node, &error)) {
                    if(error) {
                        disconnect_market_client(client); // @Cleanup @Boilerplate
                        c--;
                    }
                    break;
                }
                    
                MSB_Packet_Header header;
                MSB_Header(client, &header);
                if(client == NULL) {
                    c--;
                    break;
                }

                bool do_disconnect = false;
                    
                if(header.type == MSB_GOODBYE) {
                    // TODO @Norelease: Better logging, with more client info etc.
                    MS_Log("Client (socket = %lld) sent goodbye message.\n", client->node.socket.handle);
                    do_disconnect = true;
                }
                else if(!read_and_handle_msb_packet(client, header)) {
                    do_disconnect = true;
                }

                if (do_disconnect) { // @Cleanup @Boilerplate
                    disconnect_market_client(client);
                    c--;
                    break;
                }
                    
            }
        }


        // UPDATE (MATCH ORDERS) //
        for(int i = 0; i < ARRLEN(server->articles); i++)
        {
            Item_Type_ID article_id = (Item_Type_ID)i; 
            
            auto *article = &server->articles[article_id];
            update_article(article, server);

            if(!article->did_change) continue;

            // @Norelease: We should write the packet we want to send to a buffer,
            //             and then send the same thing to all the watchers.
            
            // SEND UPDATES TO WATCHERS // 
            for(int j = 0; j < article->watchers.n; j++)
            {
                auto client_id = article->watchers[j];
                auto *client = find_market_client(client_id, server);
                if(!client) {
                    Assert(false);
                    continue; // @Norelease: Remove this watcher?
                }

                Fail_If_True(!send_market_update_to_article_watcher(client, article_id, article, server));
            }
            
            article->did_change = false;
        }
        // ////// //
        

        add_new_market_clients(server);
        platform_sleep_milliseconds(1);
    }

    MS_Log("Stopping listening loop...");
    stop_listening_loop(listening_loop, &listening_loop_thread);
    MS_Log_No_T("Done.\n");
    
    MS_Log("Exiting.\n");

    return 0;
}
