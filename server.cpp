
double get_time() {
    return (double)platform_performance_counter() / (double)platform_performance_counter_frequency();
}

void stop_room_server(Room_Server *server, s32 timeout_ms)
{
    set(&server->should_exit, true);
    platform_join_thread(server->thread, timeout_ms);
}

void stop_user_server(User_Server *server, s32 timeout_ms)
{
    set(&server->should_exit, true);
    platform_join_thread(server->thread, timeout_ms);
}

void stop_market_server(Market_Server *server, s32 timeout_ms)
{
    set(&server->should_exit, true);
    platform_join_thread(server->thread, timeout_ms);
}

// Returned string is allocated with ALLOC_TMP or constant.
String socket_to_string(Socket socket)
{
    sockaddr_in addr = {0};
    int         addr_size = sizeof(addr);
    bool addr_valid = getpeername(socket.handle, (sockaddr *)&addr, &addr_size);
    
    auto wsa_error = WSAGetLastError();

    auto &ip = addr.sin_addr.S_un.S_un_b;
    String ip_str = concat_tmp(ip.s_b1, ".", ip.s_b2, ".", ip.s_b3, ".", ip.s_b4);
    
    return concat_tmp("(", socket.handle, " | ", ip_str, ":", addr.sin_port, " | E: ", wsa_error, ")");
}

bool start_new_room_server(Server *server)
{
    Room_Server *s = array_add_uninitialized(server->room_servers);
    Zero(*s);
    
    init_room_server(s, server->next_server_id++);
    if(!platform_create_thread(&room_server_main_loop, s, &s->thread)) {
        Debug_Print("Failed to create room server thread.\n");
        server->room_servers.n -= 1;
        return false;
    }

    return true;
}

bool start_new_user_server(Server *server)
{
    User_Server *s = array_add_uninitialized(server->user_servers);
    Zero(*s);
    
    init_user_server(s, server->next_server_id++);
    if(!platform_create_thread(&user_server_main_loop, s, &s->thread)) {
        Debug_Print("Failed to create user server thread.\n");
        server->user_servers.n -= 1;
        return false;
    }

    return true;
}

bool start_new_market_server(Server *server)
{
    Market_Server *s = array_add_uninitialized(server->market_servers);
    Zero(*s);
    
    init_market_server(s, server->next_server_id++);
    if(!platform_create_thread(&market_server_main_loop, s, &s->thread)) {
        Debug_Print("Failed to create market server thread.\n");
        server->market_servers.n -= 1;
        return false;
    }

    return true;
}

int server_entry_point(int num_args, char **arguments)
{
    Debug_Print("I am a server.\n");

    Server server = {0};
    
    if(!platform_init_socket_use()) { Debug_Print("platform_init_socket_use() failed.\n"); return 1; }
    defer(platform_deinit_socket_use(););

    server.next_server_id = 1; // @Norelease: This must be globally unique, so need to reserve it from some master ID server...

    if(!start_new_room_server(&server)) return 1;
    if(!start_new_user_server(&server)) return 1;
    if(!start_new_market_server(&server)) return 1;
    
    // TODO @Temporary @Norelease: Wait for termination signal
    WaitForSingleObject(server.room_servers[0].thread.handle, INFINITE);
    
    for(int i = 0; i < server.room_servers.n; i++)
        stop_room_server(&server.room_servers[i], 10 * 1000);
    
    for(int i = 0; i < server.user_servers.n; i++)
        stop_user_server(&server.user_servers[i], 10 * 1000);
    
    for(int i = 0; i < server.market_servers.n; i++)
        stop_market_server(&server.market_servers[i], 10 * 1000);

    
    printf("Main server thread exiting.\n");
    return 0;
}
