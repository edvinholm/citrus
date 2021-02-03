
#include "network_node.cpp"

#include "market_server_bound.h"

#include "market_server_bound.cpp"
#include "market_client_bound.cpp"


bool receive_next_mcb_packet(Network_Node *node, MCB_Packet_Header *_packet_header, bool *_error, bool block = false)
{
    if(!receive_next_network_node_packet(node, _error, block)) return false;

    *_error = true;
    Read_To_Ptr(MCB_Packet_Header, _packet_header, node);
    *_error = false;
    
    return true;
}

bool wait_for_next_mcb_packet(Network_Node *node, MCB_Packet_Header *_packet_header)
{
    bool error;
    if(!receive_next_mcb_packet(node, _packet_header, &error, true)) return false;
    return true;
}

bool expect_type_of_next_mcb_packet(MCB_Packet_Type expected_type, Network_Node *node, MCB_Packet_Header *_packet_header)
{
    if(!wait_for_next_mcb_packet(node, _packet_header)) return false;
    if(_packet_header->type != expected_type) return false;
    return true;
}



bool connect_to_market_server(User_ID user_id, Network_Node *node, MS_Client_Type client_type)
{
    Socket socket;
    
    if(!platform_create_tcp_socket(&socket)) {
        Debug_Print("Unable to create socket.\n");
        return false;
    }

    if(!platform_connect_socket(&socket, "127.0.0.1", MARKET_SERVER_PORT)) {
        Debug_Print("Unable to connect socket to market server. WSA Error: %d\n", WSAGetLastError());
        return false;
    }

    reset_network_node(node, socket);

    Send_Now(MSB_HELLO, node, user_id, client_type);
    
    platform_set_socket_read_timeout(&node->socket, 5 * 1000);

    MCB_Packet_Header header;
    
    Fail_If_True(!expect_type_of_next_mcb_packet(MCB_HELLO, node, &header));
    Fail_If_True(header.hello.connect_status != MARKET_CONNECT__CONNECTED);

    platform_set_socket_read_timeout(&node->socket, 1000);

    return true;
}


bool disconnect_from_market_server(Network_Node *node, bool say_goodbye = true)
{    
    bool result = true;

    if(say_goodbye)
    {
        // REMEMBER: This is special, so don't do anything fancy here that can put us in an infinite disconnection loop.
        if(!send_MSB_GOODBYE_packet_now(node)) result = false;

        //TODO @Norelease @Robustness: What if the server sends another packet inbetween?
        //                             We should have a way of skipping all packets until
        //                             a specific type. (Here, that type is MCB_GOODBYE).
        MCB_Packet_Header header;
        bool error;
        if(!expect_type_of_next_mcb_packet(MCB_GOODBYE, node, &header))
        {
            result = false;
        }
    }
    
    if(!platform_close_socket(&node->socket)) result = false;
    
    return result;
}
