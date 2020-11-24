
// IMPORTANT: There is one definition for this for the client, and another for the server.
#define Fail_If_True(Condition) \
    if(Condition) { Debug_Print("[FAILURE] Condition met: "); Debug_Print(#Condition); Debug_Print("\n"); return false; }


const int LISTENING_SOCKET_BACKLOG_SIZE = 1024;

struct Server {
    Room_Server room_server;
    User_Server user_server;
};


double get_time();
