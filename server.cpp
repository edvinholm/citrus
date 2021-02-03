
double get_time() {
    return (double)platform_performance_counter() / (double)platform_performance_counter_frequency();
}

void stop_room_server(Room_Server *server, Thread *thread, s32 timeout_ms)
{
    set(&server->should_exit, true);
    platform_join_thread(*thread, timeout_ms);
}

void stop_user_server(User_Server *server, Thread *thread, s32 timeout_ms)
{
    set(&server->should_exit, true);
    platform_join_thread(*thread, timeout_ms);
}

void stop_market_server(Market_Server *server, Thread *thread, s32 timeout_ms)
{
    set(&server->should_exit, true);
    platform_join_thread(*thread, timeout_ms);
}

int server_entry_point(int num_args, char **arguments)
{
    Debug_Print("I am a server.\n");

    Server server = {0};
    Room_Server   *room_server   = &server.room_server;
    User_Server   *user_server   = &server.user_server;
    Market_Server *market_server = &server.market_server;
        
    if(!platform_init_socket_use()) { Debug_Print("platform_init_socket_use() failed.\n"); return 1; }
    defer(platform_deinit_socket_use(););

    u32 next_server_index = 1; // @Norelease: This must be globally unique, so need to reserve it from some master ID server...
    
    // INIT ROOM SERVER //
    init_room_server(room_server, next_server_index++);
    Thread room_server_thread;
    if(!platform_create_thread(&room_server_main_loop, room_server, &room_server_thread)) {
        Debug_Print("Failed to create room server thread.\n");
        return 1;
    }
    defer(stop_room_server(room_server, &room_server_thread, 10*1000););

    // INIT USER SERVER //
    init_user_server(user_server, next_server_index++);
    Thread user_server_thread;
    if(!platform_create_thread(&user_server_main_loop, user_server, &user_server_thread)) {
        Debug_Print("Failed to create user server thread.\n");
        return 1;
    }
    defer(stop_user_server(user_server, &user_server_thread, 10*1000););

    // INIT MARKET SERVER //
    init_market_server(market_server, next_server_index++);
    Thread market_server_thread;
    if(!platform_create_thread(&market_server_main_loop, market_server, &market_server_thread)) {
        Debug_Print("Failed to create market server thread.\n");
        return 1;
    }
    defer(stop_market_server(market_server, &market_server_thread, 10*1000););

    
    // TODO @Temporary @Norelease: Wait for termination signal
    WaitForSingleObject(room_server_thread.handle, INFINITE);

    
    printf("Main server thread exiting.\n");
    return 0;
}
