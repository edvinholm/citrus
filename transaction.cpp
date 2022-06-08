


Transaction *create_transaction(Network_Node *node)
{
    Transaction t = {0};
    node->last_transaction_id++;
    t.id = node->last_transaction_id;

    t.incoming = false;

    return array_add(node->transaction_queue, t);
}
