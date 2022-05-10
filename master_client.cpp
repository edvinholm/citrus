

#include "network_node.cpp"


bool connect_to_master_server(Network_Node *node)
{
    Assert(client_type == XS_CLIENT_PLAYER || server_id > 0);
    
    Socket socket;
    
    if(!platform_create_tcp_socket(&socket)) {
        Debug_Print("Unable to create socket.\n");
        return false;
    }

    if(!platform_connect_socket(&socket, "127.0.0.1", MASTER_SERVER_PORT)) {
        Debug_Print("Unable to connect socket to user server. WSA Error: %d\n", WSAGetLastError());
        return false;
    }

    reset_network_node(node, socket);

    Send_Now(XSB_HELLO, node, client_type, server_id);
    
    platform_set_socket_read_timeout(&node->socket, 5 * 1000);

    XCB_Packet_Header header;
    
    Fail_If_True(!expect_type_of_next_xcb_packet(XCB_HELLO, node, &header));
    Fail_If_True(header.hello.connect_status != MASTER_CONNECT__CONNECTED);

    platform_set_socket_read_timeout(&node->socket, 1000);

    return true;    
}


bool disconnect_from_user_server(Network_Node *node, bool say_goodbye = true)
{    
    bool result = true;

    if(say_goodbye)
    {
        // REMEMBER: This is special, so don't do anything fancy here that can put us in an infinite disconnection loop.
        if(!send_USB_GOODBYE_packet_now(node)) result = false;

        //TODO @Norelease @Robustness: What if the server sends another packet inbetween?
        //                             We should have a way of skipping all packets until
        //                             a specific type. (Here, that type is UCB_GOODBYE).
        UCB_Packet_Header header;
        bool error;
        if(!expect_type_of_next_ucb_packet(UCB_GOODBYE, node, &header))
        {
            result = false;
        }
    }
    
    if(!platform_close_socket(&node->socket)) result = false;
    
    return result;
}

