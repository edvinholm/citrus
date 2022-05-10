
bool receive_next_rcb_packet(Network_Node *node, RCB_Packet_Header *_packet_header, bool *_error, bool block = false);
bool expect_type_of_next_rcb_packet(RCB_Packet_Type expected_type, Network_Node *node, RCB_Packet_Header *_packet_header);
