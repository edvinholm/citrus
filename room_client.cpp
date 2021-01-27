
#include "network_node.cpp"
#include "room_server_bound.cpp"
#include "room_client_bound.cpp"

bool receive_next_rcb_packet(Network_Node *node, RCB_Packet_Header *_packet_header, bool *_error, bool block = false)
{
    if(!receive_next_network_node_packet(node, _error, block)) return false;
    
    *_error = true;
    Read_To_Ptr(RCB_Packet_Header, _packet_header, node);
    *_error = false;
    
    return true;
}

bool expect_type_of_next_rcb_packet(RCB_Packet_Type expected_type, Network_Node *node, RCB_Packet_Header *_packet_header)
{
    bool error;
    if(!receive_next_rcb_packet(node, _packet_header, &error, true)) return false;
    if(_packet_header->type != expected_type) return false;
    return true;
}

bool connect_to_room_server(Room_ID room, User_ID as_user, Network_Node *node)
{
    Socket socket;
    
    if(!platform_create_tcp_socket(&socket)) {
        Debug_Print("Unable to create socket.\n");
        return false;
    }

    if(!platform_connect_socket(&socket, "127.0.0.1", ROOM_SERVER_PORT)) {
        Debug_Print("Unable to connect socket to room server. WSA Error: %d\n", WSAGetLastError());
        return false;
    }

    reset_network_node(node, socket);

    Send_Now(RSB_HELLO, node, room, as_user);
                 
    platform_set_socket_read_timeout(&node->socket, 5 * 1000);

    RCB_Packet_Header header;
    
    Fail_If_True(!expect_type_of_next_rcb_packet(RCB_HELLO, node, &header));
    Fail_If_True(header.hello.connect_status != ROOM_CONNECT__REQUEST_RECEIVED);
    
    Fail_If_True(!expect_type_of_next_rcb_packet(RCB_HELLO, node, &header));
    Fail_If_True(header.hello.connect_status != ROOM_CONNECT__CONNECTED);

    platform_set_socket_read_timeout(&node->socket, 1000);

    return true;
}


bool disconnect_from_room_server(Network_Node *node, bool say_goodbye = true)
{
    bool result = true;

    if(say_goodbye)
    {
        bool goodbye_success = true;
        
        // REMEMBER: This is special, so don't do anything fancy here that can put us in an infinite disconnection loop.
        if(goodbye_success && !send_RSB_GOODBYE_packet_now(node))
            goodbye_success = false;

        RCB_Packet_Header header;
        if(goodbye_success && !expect_type_of_next_rcb_packet(RCB_GOODBYE, node, &header))
            goodbye_success = false;

        if(!goodbye_success)
            result = false;
    }
    
    if(!platform_close_socket(&node->socket)) result = false;
    
    return result;
}
