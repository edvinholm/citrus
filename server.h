

struct Server {
    Array<Room_Server, ALLOC_MALLOC>   room_servers;
    Array<User_Server, ALLOC_MALLOC>   user_servers;
    Array<Market_Server, ALLOC_MALLOC> market_servers;

    u32 next_server_id;
};


double get_time();


String socket_to_string(Socket socket);
