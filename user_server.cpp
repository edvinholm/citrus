
#define US_Log_T(Tag, ...) \
    printf("[-US-]");                \
    Global_Log_T(Tag, __VA_ARGS__)
#define US_Log(...)                     \
    Global_Log_T("-US-", __VA_ARGS__)


void init_user_server(User_Server *server)
{
    init_atomic(&server->should_exit);
}

void deinit_user_server(User_Server *server)
{
    deinit_atomic(&server->should_exit);
}

// REMEMBER to init_user_server before starting this.
//          IMPORTANT: Do NOT deinit_user_server after
//                     this is done -- this proc will do that for you.
DWORD user_server_main_loop(void *server_) {
    
    User_Server *server = (User_Server *)server_;
    defer(deinit_user_server(server););
    
    US_Log("User server running.\n");
    while(!get(&server->should_exit)) {
        platform_sleep_milliseconds(100);
    }
    US_Log("User server exiting.\n");

    return 0;
}
