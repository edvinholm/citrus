
enum Server_Type {
    SERVER_MASTER,
    SERVER_ROOM,
    SERVER_USER,
    SERVER_MARKET,

    SERVER_NONE_OR_NUM
};
String server_type_names[] = {
    STRING("Master"),
    STRING("Room"),
    STRING("User"),
    STRING("Market")
};
static_assert(ARRLEN(server_type_names) == SERVER_NONE_OR_NUM);

struct Server_Info {
    Server_Type type;
    u16 port;
    u16 local_id; // Local to the "physical" server.
};


void dev_servers_view(UI_Context ctx)
{
    U(ctx);
    
    Server_Info servers[] = {
        { SERVER_MASTER, MASTER_SERVER_PORT, 1 },
        { SERVER_USER, USER_SERVER_PORT, 1  },
        { SERVER_ROOM, ROOM_SERVER_PORT, 9  },
        { SERVER_ROOM, ROOM_SERVER_PORT, 27 },
        { SERVER_ROOM, ROOM_SERVER_PORT, 3  },
        { SERVER_MARKET, MARKET_SERVER_PORT, 1 }
    };
    
    String titles[]    = { STRING("Type"), STRING("Address"), STRING("Port"), STRING("Local ID") };
    float  fractions[] = { .20, .40, .20, .20 };
    static_assert(ARRLEN(titles) == ARRLEN(fractions));
    begin_list(P(ctx), titles, fractions, ARRLEN(titles));
    {
        for(int i = 0; i < ARRLEN(servers); i++) {
            auto *it = &servers[i];
            list_cell(PC(ctx, i), server_type_names[it->type]);
            list_cell(PC(ctx, i), EMPTY_STRING);
            list_cell(PC(ctx, i), concat_tmp("", it->port));
            list_cell(PC(ctx, i), concat_tmp("", it->local_id));
        }
    }
    end_list(P(ctx));

}
