bool receive_next_mcb_packet(Network_Node *node, MCB_Packet_Header *_packet_header, bool *_error, bool block = false);
bool wait_for_next_mcb_packet(Network_Node *node, MCB_Packet_Header *_packet_header);
bool expect_type_of_next_mcb_packet(MCB_Packet_Type expected_type, Network_Node *node, MCB_Packet_Header *_packet_header);
