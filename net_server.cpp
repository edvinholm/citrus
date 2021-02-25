
const int DEFAULT_SOCKET_BACKLOG_LENGTH = 1024;

bool setup_listening_socket(Socket *_sock, u16 port, bool blocking, int max_backlog_length)
{
    Fail_If_True(!platform_create_tcp_socket(_sock, blocking));
    
    if(!platform_bind_socket(_sock, port)) {
        Debug_Print("Unable to bind listening socket to port %u. Error: %d.\n", port, WSAGetLastError());
        return false;
    }
    
    Fail_If_True(!platform_start_listening_to_socket(_sock, max_backlog_length));
    
    return true;
}

// NOTE: Zero *loop before calling this.
bool init_listening_loop(Listening_Loop *loop, u16 port, int max_backlog_length = DEFAULT_SOCKET_BACKLOG_LENGTH)
{
    init_atomic(&loop->should_exit);
    init_atomic(&loop->client_accept_failed);

    return setup_listening_socket(&loop->socket, port, false, max_backlog_length);
}

// NOTE: Just deinits os handles etc. Does not free memory.
void deinit_listening_loop(Listening_Loop *loop)
{
    deinit_atomic(&loop->should_exit);
    deinit_atomic(&loop->client_accept_failed);

    if(!platform_close_socket(&loop->socket))
    {
        Debug_Print("platform_close_socket failed. (WSA Error: %d)\n", WSAGetLastError());
    }
}


bool start_listening_loop(Listening_Loop *loop, u16 port,
                          DWORD (*proc)(void *), void *proc_param, Thread *thread,
                          const char *log_tag, int max_backlog_length = DEFAULT_SOCKET_BACKLOG_LENGTH)
{
    while(!init_listening_loop(loop, port, max_backlog_length))
    {
        Log_T(log_tag, "init_listening_loop() failed. Retrying in 3...");
        platform_sleep_milliseconds(1000);
        Log_T(log_tag, "2..."); platform_sleep_milliseconds(1000);
        Log_T(log_tag, "1..."); platform_sleep_milliseconds(1000);
        Log_T(log_tag, "\n");
    }
    if(!platform_create_thread(proc, proc_param, thread)) {
        Log_T(log_tag, "Unable to create listening loop thread.\n");
        return false;
    }
    return true;
}


void stop_listening_loop(Listening_Loop *loop, Thread *thread, s32 timeout_ms = -1)
{
    set(&loop->should_exit, true);
    platform_join_thread(*thread, timeout_ms);
    deinit_listening_loop(loop);
}


bool listening_loop_running(Listening_Loop *loop, bool client_socket_block_mode, bool *_client_accepted, Socket *_client_socket, const char *log_tag)
{
    *_client_accepted = false;
    
    if(get(&loop->should_exit)) {
        return false;
    }

    bool error;
    if(platform_accept_next_incoming_socket_connection(&loop->socket, client_socket_block_mode, _client_socket, &error)) {
        // @Norelease: TODO: Get IP address.
        *_client_accepted = true;
    }
    else
    {
        if(!error) {
            // We should only get here if the socket is non-blocking.
            platform_sleep_milliseconds(100);
            return true;
        }

        Log_T(log_tag, "Listening loop client accept failure\n");
        set(&loop->client_accept_failed, true);
        
        return false;
    }
    
    return true;
}
