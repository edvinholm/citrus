
#define Enqueue(Packet_Type, Node_Ptr, ...) \
    Fail_If_True(!enqueue_##Packet_Type##_packet(Node_Ptr, __VA_ARGS__));   \
    
#define Enqueue_NoArgs(Packet_Type, Node_Ptr)                        \
    Fail_If_True(!enqueue_##Packet_Type##_packet(Node_Ptr));    \
    
#define Send_Now(Packet_Type, Node_Ptr, ...)                            \
    Enqueue(Packet_Type, Node_Ptr, __VA_ARGS__);                        \
    Assert((Node_Ptr)->packet_queue.n == 1);                            \
    Fail_If_True(!send_outbound_packets(Node_Ptr))

// @Jai @Cleanup this stupid poop.
#define Send_Now_NoArgs(Packet_Type, Node_Ptr)                  \
    Enqueue_NoArgs(Packet_Type, Node_Ptr);                        \
    Assert((Node_Ptr)->packet_queue.n == 1);                      \
    Fail_If_True(!send_outbound_packets(Node_Ptr))


#define Data_Ptr(Ptr, Length, Node_Ptr)                                 \
    Fail_If_True(!place_data_pointer((u8 **)&Ptr, Length, Node_Ptr))


struct Network_Node_Buffer: public Memory_Buffer
{
    s64 caret; // Only used when reading.
    s64 packet_size; // When sending, this is the size of all packets together.
};

struct Network_Node
{
    // TODO @Norelease: Don't have the buffers in the node struct.
    //                  Have pointers to them here instead, or something...
    //                  and share them between multiple nodes where possible.
    Network_Node_Buffer receive_buffer; // NOTE: This holds at most one packet.
    Network_Node_Buffer send_buffer;    // NOTE: This holds at most one packet.

    struct Packet {
        s64 start;
        s64 size;
    };
    
    s64 current_outbound_packet_start_plus_one;
    Array<Packet, ALLOC_MALLOC> packet_queue;

    bool connected;
    bool last_connect_attempt_failed;
    Socket socket;

    Array<Transaction, ALLOC_MALLOC> transaction_queue;
    bool first_transaction_in_queue_is_active;
    Transaction_ID last_transaction_id;
};

bool equal(Network_Node *a, Network_Node *b)
{
    if(a->connected != b->connected) return false;
    if(a->last_connect_attempt_failed != b->last_connect_attempt_failed) return false;
    if(!equal(&a->socket, &b->socket)) return false;
    return true;
}




bool can_read_bytes(s64 length, Network_Node *node);
bool read_bytes(void *_data, s64 length, Network_Node *node);
bool place_data_pointer(u8 **_pointer, s64 data_length, Network_Node *node);

bool read_s8(s8 *_i, Network_Node *node);
bool read_s16(s16 *_i, Network_Node *node);
bool read_s32(s32 *_i, Network_Node *node);
bool read_s64(s64 *_i, Network_Node *node);
bool read_u8(u8 *_i, Network_Node *node);
bool read_u16(u16 *_i, Network_Node *node);
bool read_u32(u32 *_i, Network_Node *node);
bool read_u64(u64 *_i, Network_Node *node);
bool read_float(float *_f, Network_Node *node);
bool read_bool(bool *_b, Network_Node *node);
bool read_Quat(Quat *_q, Network_Node *node);
bool read_v3(v3 *_u, Network_Node *node);
bool read_v4(v4 *_u, Network_Node *node);
bool read_String(String *_str, Network_Node *node);
    
bool write_bytes(void *data, u64 length, Network_Node *node);
bool write_s8(s8 i, Network_Node *node);
bool write_s16(s16 i, Network_Node *node);
bool write_s32(s32 i, Network_Node *node);
bool write_s64(s64 i, Network_Node *node);
bool write_u8(u8 i, Network_Node *node);
bool write_u16(u16 i, Network_Node *node);
bool write_u32(u32 i, Network_Node *node);
bool write_u64(u64 i, Network_Node *node);
bool write_float(float f, Network_Node *node);
bool write_bool(bool b, Network_Node *node);
bool write_Quat(Quat q, Network_Node *node);
bool write_v3(v3 u, Network_Node *node);
bool write_v4(v4 u, Network_Node *node);
bool write_String(String str, Network_Node *node);

bool write_network_packet(u8 *payload, u64 size, Socket *sock);
void begin_outbound_packet(Network_Node *node);
bool end_outbound_packet(Network_Node *node);
bool send_outbound_packets(Network_Node *node);
