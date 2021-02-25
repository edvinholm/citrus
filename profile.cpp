
void next_profiler_frame(Profiler *profiler)
{
    if(!tweak_bool(TWEAK_RUN_PROFILER)) return;
    
    if(!profiler->paused) {
        profiler->frame_cursor += 1;
        profiler->frame_cursor %= (sizeof(profiler->frames)/sizeof(profiler->frames[0]));
    }

    auto *frame = &profiler->frames[profiler->frame_cursor];
    memset(frame, 0, sizeof(*frame));
}

void push_profiler_node(Profiler *profiler, const char *label)
{
    if(!tweak_bool(TWEAK_RUN_PROFILER)) return;

    auto *frame = &profiler->frames[profiler->frame_cursor];

    Assert(frame->num_nodes < MAX_PROFILER_FRAME_NODES);
    auto node_ix = frame->num_nodes;
    
    auto *node = &frame->nodes[node_ix];
    memset(node, 0, sizeof(*node));
    
    if(frame->current_node != NULL) {
        auto *parent = frame->current_node;
        
        Assert(parent->num_children < MAX_PROFILER_NODE_CHILDREN);
        parent->children[parent->num_children++] = node;

        node->parent = parent;
    }

    node->label = label;
    
    frame->current_node = node;
    frame->num_nodes++;

    node->t0 = platform_performance_counter();
}

Profiler_Node *pop_profiler_node(Profiler *profiler, s32 user_int/* = 0*/, float user_float/* = 0*/)
{
    if(!tweak_bool(TWEAK_RUN_PROFILER)) return NULL;

    s64 t1 = platform_performance_counter();
    
    auto *frame = &profiler->frames[profiler->frame_cursor];

    auto *node = frame->current_node;
    Assert(node);

    node->t1 = t1;
    node->user_int = user_int;
    
    frame->current_node = node->parent;

    return node;
}

