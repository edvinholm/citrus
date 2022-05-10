


bool receive_next_ucb_packet(Network_Node *node, UCB_Packet_Header *_packet_header, bool *_error, bool block = false);
bool wait_for_next_ucb_packet(Network_Node *node, UCB_Packet_Header *_packet_header);
bool expect_type_of_next_ucb_packet(UCB_Packet_Type expected_type, Network_Node *node, UCB_Packet_Header *_packet_header);
