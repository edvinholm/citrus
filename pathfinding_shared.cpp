
#define DEBUG_PATHFINDING (DEBUG) && FALSE

#if DEBUG_PATHFINDING
#define PF_Expensive_Assert(X) Assert(X)
#define PF_Assert_Tree_Valid(...) assert_tree_valid(__VA_ARGS__)
#else
#define PF_Expensive_Assert(X)
#define PF_Assert_Tree_Valid(...)
#endif


double player_walk_path_duration(v3 *path, u16 length)
{
    double t = 0;
    for(int i = 1; i < length; i++)
        t += magnitude(path[i] - path[i-1]) / player_walk_speed;
    
    return t;
}


template<Allocator_ID A>
void remove_unnecessary_walk_path_nodes(Array<v3, A> *path)
{
    Function_Profile();
    
    for(int i = 1; i < path->n-1; i++)
    {
        v3 a = (*path)[i-1];
        v3 b = (*path)[i];
        v3 c = (*path)[i+1];
        
        v3 ab_dir = normalize(b - a);
        v3 bc_dir = normalize(c - b);

        if(is_zero(magnitude(bc_dir - ab_dir)))
        {       
            array_ordered_remove((*path), i);
            i--;
        }
    }
}


Pathfinding_Node *find_smallest(Pathfinding_Node *root)
{
    if (root == NULL) return NULL;

    auto *result = root;
    while(result->o_left)
        result = result->o_left;

    return result;
}

#if DEBUG_PATHFINDING
void assert_tree_valid(Pathfinding_Node *root, Pathfinding_Node *parent)
{
    if(root == NULL) return;

    Assert(root->o_parent == parent);

    Assert(root->list_status == Pathfinding_Node::IN_OPEN);
    Assert(root->times_visited == 0);
    

    if (root->o_left) {
        if (root->o_left->f_cost >= root->f_cost) Assert(false);
        assert_tree_valid(root->o_left, root);
    }

    if (root->o_right) {
        if (root->o_right->f_cost < root->f_cost) Assert(false);
        assert_tree_valid(root->o_right, root);
    }
}
#endif



bool in_tree(Pathfinding_Node *node, Pathfinding_Node *root)
{
    if(root == NULL) return false;
    if(root == node) return true;
    if(in_tree(node, root->o_left))  return true;
    if(in_tree(node, root->o_right)) return true;
    return false;
}


// IMPORTANT: *_path needs to be in a valid state, and _path->n should be set to zero.
template<Allocator_ID A>
bool find_path(v3s start, v3s end, Walk_Map *walk_map, bool do_reduce_path, Array<v3, A> *_path)
{
    s32 num_visited_nodes = 0;
    push_profiler_node(PROFILER, __FUNCTION__);
    defer({
            auto *pn = pop_profiler_node(PROFILER, num_visited_nodes);
            if(pn) {
                s64 dur = pn->t1 - pn->t0;
                double ns_dur = 1000.0 * 1000.0 * 1000.0 * (double)dur/platform_performance_counter_frequency();
                pn->user_float = ns_dur/num_visited_nodes;
            }
        });

#if DEBUG_PATHFINDING
    const auto *INVALID_PARENT = (Pathfinding_Node *)(u64)0xff00ff00;
    const auto *INVALID_LEFT   = (Pathfinding_Node *)(u64)0xf00dbeef;
    const auto *INVALID_RIGHT  = (Pathfinding_Node *)(u64)0xd0ffd0ff;

#endif
    
    Assert(_path->n == 0);

    if (start == end) {
        array_add_uninitialized(*_path, 2);
        (*_path)[0] = V3(start);
        (*_path)[1] = V3(end);
        return true;
    }
    
    v2 end_p = { (float)end.x, (float)end.y };

    const auto num_nodes = ARRLEN(walk_map->nodes);
    
    Pathfinding_Node nodes[num_nodes];
    Zero(nodes);

    float f_costs[num_nodes];
    float g_costs[num_nodes];

    Pathfinding_Node *start_node = &nodes[start.y * room_size_x + start.x];
    Pathfinding_Node *end_node   = &nodes[end.y   * room_size_x + end.x];

#if DEBUG_PATHFINDING
    for(int k = 0; k < ARRLEN(nodes); k++)
    {
        auto *n = &nodes[k];
        n->times_visited = 0;
        n->came_from = U16_MAX;
        n->g_cost = -999;
        n->f_cost = -777;
        n->o_parent = INVALID_PARENT;
        n->o_left   = INVALID_LEFT;
        n->o_right  = INVALID_RIGHT;
    }
#endif

    g_costs[start_node - nodes] = 0;
    f_costs[start_node - nodes] = 0;
    
    Pathfinding_Node *open_root = start_node;    
    start_node->list_status = Pathfinding_Node::IN_OPEN;

    Pathfinding_Node *smallest_in_open = open_root;

    bool success = false;

    PF_Assert_Tree_Valid(open_root, NULL);
    

    while(true)
    {
        Assert((open_root == NULL) == (smallest_in_open == NULL));
        PF_Assert_Tree_Valid(open_root, NULL);
    
        if(open_root == NULL) break;

        Pathfinding_Node *current = smallest_in_open;
        num_visited_nodes++;

        PF_Expensive_Assert(smallest_in_open == find_smallest(open_root));

        // SET NEW SMALLEST, REMOVE FROM OPEN LIST, ADD TO CLOSED LIST //

        auto *right = current->o_right;
        if(current->o_parent == NULL)
        {
            if(right) right->o_parent = NULL;
            open_root = right;
            smallest_in_open = find_smallest(open_root);

            PF_Expensive_Assert(smallest_in_open == find_smallest(open_root));
        }
        else {
            Assert(current->o_parent->o_left == current);

            if(right) right->o_parent = current->o_parent;
            current->o_parent->o_left = current->o_right;

            if(right) smallest_in_open = find_smallest(right);
            else      smallest_in_open = current->o_parent;

            PF_Expensive_Assert(smallest_in_open == find_smallest(open_root));
        }
        PreFetchCacheLine(PF_TEMPORAL_LEVEL_1, smallest_in_open);
        
        current->list_status = Pathfinding_Node::IN_CLOSED;
        // --
        PF_Assert_Tree_Valid(open_root, NULL);

        div_t current_xy_div = div((s32)(current - nodes), room_size_x);
        float current_x = current_xy_div.rem;
        float current_y = current_xy_div.quot;

        if(current == end_node) {
            // PATH FOUND //
            PF_Assert_Tree_Valid(open_root, NULL);

            Scoped_Profile("Trace back");
            
            // Find length
            int path_length = 1;
            Pathfinding_Node *n = current;
            while(true)
            {
                Pathfinding_Node *came_from = &nodes[n->came_from];
                path_length++;
                if(came_from == start_node) break;
                n = came_from;
            }

            array_add_uninitialized(*_path, path_length);
            
            v3 current_p = { (float)(current_x), (float)(current_y), 0 };
            (*_path)[path_length-1] = current_p;
            
            int path_node_ix = 1;
            n = current;
            while(true)
            {
                Pathfinding_Node *came_from = &nodes[n->came_from];

                div_t came_from_xy_div = div((s32)(came_from - nodes), room_size_x);
                auto came_from_x = came_from_xy_div.rem;
                auto came_from_y = came_from_xy_div.quot;
                
                v3 came_from_p = { (float)(came_from_x), (float)(came_from_y), 0 };
                (*_path)[path_length-1-path_node_ix] = came_from_p;
                
                if(came_from == start_node) break;
                
                n = came_from;
                path_node_ix++;
            }

            if(do_reduce_path) {
                // Remove nodes that are not needed.
                remove_unnecessary_walk_path_nodes(_path);
            }

            success = true;
            break;
        }

        // The order of these is IMPORTANT.
        Pathfinding_Node *neighbours[] = {
            current - room_size_x, // North
            current - 1,           // West
            current + room_size_x, // South
            current + 1,           // East
            
            current - room_size_x - 1, // North-West
            current + room_size_x - 1, // West-South
            current + room_size_x + 1, // South-East
            current - room_size_x + 1  // East-North
        };

        for(int i = 0; i < ARRLEN(neighbours); i++)
        {
            PF_Assert_Tree_Valid(open_root, NULL);
        
            auto *neighbour = neighbours[i];
            if(neighbour < nodes || neighbour >= nodes + ARRLEN(nodes)) continue; // Outside the map.

            div_t neighbour_xy_div = div((s32)(neighbour - nodes), room_size_x);
            float neighbour_x = neighbour_xy_div.rem;
            float neighbour_y = neighbour_xy_div.quot;

            if (fabs(neighbour_x - current_x) > 1) continue;
            if (fabs(neighbour_y - current_y) > 1) continue;

            if(neighbour->list_status == Pathfinding_Node::IN_CLOSED) continue;

            // TODO: Check if we can walk over the edge. Have a walk map with edges, or have disallowed edges as flags in the Walk_Mask.

            auto neighbour_ix = (neighbour - nodes);
            if(walk_map->nodes[neighbour_ix].flags & UNWALKABLE) continue;

            bool diagonal = (i >= 4);

            if(diagonal) {
                // Check that "neighbours" to the diagonal line are walkable.
                
                Pathfinding_Node *a = neighbours[(((i-4) % 4) + 4) % 4];
                Pathfinding_Node *b = neighbours[(((i-3) % 4) + 4) % 4];
                // These should always be inside the map if this diagonal neighbour is.

                auto a_ix = (a - nodes);
                auto b_ix = (b - nodes);

                if(walk_map->nodes[a_ix].flags & UNWALKABLE) continue;
                if(walk_map->nodes[b_ix].flags & UNWALKABLE) continue;
            }
            PF_Assert_Tree_Valid(open_root, NULL);
            
            float rel_g_cost = (diagonal) ? 1.4f : 1.0f;
            float g_cost = g_costs[current-nodes] + rel_g_cost;

            float dx = end_p.x - neighbour_x;
            float dy = end_p.y - neighbour_y;
            float h_cost = sqrtf(dx*dx + dy*dy);

            float f_cost = g_cost + h_cost;
            
            if(neighbour->list_status != Pathfinding_Node::IN_OPEN || f_cost < f_costs[neighbour_ix])
            {                
                // ADD TO OR SORT OPEN LIST //
                // (Sort by f_cost) //


                // IMPORTANT: old_parent only valid if old_list_status == IN_OPEN!
                auto old_list_status = neighbour->list_status;
                Pathfinding_Node *old_parent;
                if(old_list_status == Pathfinding_Node::IN_OPEN)
                    old_parent = neighbour->o_parent;
#if DEBUG_PATHFINDING
                if(old_list_status != Pathfinding_Node:IN_OPEN)
                    old_parent = INVALID_PARENT;
#endif
                

                // Remove
                if(neighbour->list_status == Pathfinding_Node::IN_OPEN)
                {
                    PF_Assert_Tree_Valid(open_root, NULL);
                    PF_Expensive_Assert(in_tree(neighbour, open_root));
                    PF_Expensive_Assert(neighbour->o_parent == NULL || in_tree(neighbour->o_parent, open_root));
                    Assert(neighbour->o_parent != NULL || neighbour == open_root);

                    auto *parent = neighbour->o_parent;

                    Pathfinding_Node **parent_child_ptr = NULL;
                    if(parent == NULL) {
                        parent_child_ptr = &open_root;
                    }
                    else if(parent->o_left == neighbour)  parent_child_ptr = &parent->o_left;
                    else if(parent->o_right == neighbour) parent_child_ptr = &parent->o_right;
                    else { Assert(false); }
                    
                    PF_Assert_Tree_Valid(neighbour, neighbour->o_parent);
                    PF_Assert_Tree_Valid(open_root, NULL);

                    auto *left  = neighbour->o_left;
                    auto *right = neighbour->o_right;
                    if(left == NULL && right == NULL) {

                        *parent_child_ptr = NULL;
                        
                        PF_Assert_Tree_Valid(open_root, NULL);
                        
                    } else if(left == NULL) {
                        right->o_parent = parent;
                        *parent_child_ptr = right;

                        PF_Assert_Tree_Valid(right, right->o_parent);
                        PF_Assert_Tree_Valid(open_root, NULL);
                        
                    } else if(right == NULL) {
                        left->o_parent = parent;
                        *parent_child_ptr = left;
                        
                        PF_Assert_Tree_Valid(left, left->o_parent);
                        PF_Assert_Tree_Valid(open_root, NULL);
        
                    } else {
                        Pathfinding_Node *sir = find_smallest(right);

                        Assert(sir->o_parent != NULL); // Because right has a parent.
                        Assert(sir == right || sir->o_parent->o_left == sir); // Either we are the root of the right subtree, or we are a left child, because right children can't be smaller than their parent.
                        Assert(sir->o_left == NULL); // We are the smallest in the right subtree, so we can't have children on our left side.

                        // If SIR == right, this is easy.
                        if (sir == right) {
                            Assert(right->o_left == NULL);

                            neighbour->o_left->o_parent = right;
                            right->o_left = neighbour->o_left;
                            
                            *parent_child_ptr = right;
                            right->o_parent = parent;

#if DEBUG_PATHFINDING
                            // NOTE: We don't need to set these to NULL, because neighbour
                            // will be removed from the tree.
                            neighbour->o_parent = INVALID_PARENT; 
                            neighbour->o_left   = INVALID_LEFT; 
                            neighbour->o_right  = INVALID_RIGHT;
#endif

                            PF_Assert_Tree_Valid(open_root, NULL);
                            PF_Assert_Tree_Valid(sir, sir->o_parent);
                            PF_Expensive_Assert(in_tree(sir, open_root));
                        }
                        else {
                            // REMOVE SIR
                            // We know that SIR has a parent and that SIR is the
                            // left child, since it's the smallest in the right subtree...

                            // SIR might still have a child tree on the right, so we connect
                            // that (or NULL) as the left tree of SIR's parent.
                            sir->o_parent->o_left  = sir->o_right;
                            if(sir->o_right) sir->o_right->o_parent = sir->o_parent;

#if DEBUG_PATHFINDING
                            // NOTE: SIR's parent will be set to something else later,
                            //       so we don't need to NULL it here.
                            sir->o_parent = INVALID_PARENT;
#endif

                            Assert(sir->o_left == NULL);
                            // ///////

                            // Replace neighbour's parent's child with SIR.
                            Assert((neighbour->o_parent == NULL) == (parent_child_ptr == &open_root));
                            *parent_child_ptr = sir;
                            sir->o_parent = neighbour->o_parent;

                            // Replace neighbour's childen's parent with SIR.
                            Assert(neighbour->o_left && neighbour->o_right);
                            neighbour->o_left->o_parent  = sir;
                            neighbour->o_right->o_parent = sir;
                            sir->o_left  = neighbour->o_left;
                            sir->o_right = neighbour->o_right;    

#if DEBUG_PATHFINDING
                            // NOTE: Neighbour will be removed from the tree,
                            //       so we don't need to NULL these here.
                            neighbour->o_parent = INVALID_PARENT;
                            neighbour->o_left   = INVALID_LEFT; 
                            neighbour->o_right  = INVALID_RIGHT;
#endif
                            // /////////

                            PF_Assert_Tree_Valid(open_root, NULL);
                            PF_Assert_Tree_Valid(sir, sir->o_parent);
                            PF_Expensive_Assert(in_tree(sir, open_root));
                        }

                    }

                    PF_Expensive_Assert(!in_tree(neighbour, open_root));
                    neighbour->list_status = Pathfinding_Node::IN_NO_LIST;
                    
                    PF_Assert_Tree_Valid(open_root, NULL);
                }


                // UPDATE NODE //
                g_costs[neighbour_ix] = g_cost;
                f_costs[neighbour_ix] = f_cost;
                neighbour->came_from = (current - nodes);
                // //////////////// //
                
                // INSERT //
                neighbour->o_left  = NULL;
                neighbour->o_right = NULL;

                if(open_root == NULL)
                {
                    // This branch is taken ~0.2% of the time (2021-02-24)

                    neighbour->o_parent = NULL;
                    open_root = neighbour;

                    smallest_in_open = open_root;
                    PF_Expensive_Assert(smallest_in_open == find_smallest(open_root));
                    PreFetchCacheLine(PF_TEMPORAL_LEVEL_1, smallest_in_open);

                    neighbour->list_status = Pathfinding_Node::IN_OPEN;
                    
                    PF_Assert_Tree_Valid(neighbour, neighbour->o_parent);
                    PF_Assert_Tree_Valid(open_root, NULL);
                }
                else if (f_cost < f_costs[smallest_in_open-nodes])
                {
                    // This branch is taken ~4.4% of the time (2021-02-24)
                    
                    PF_Expensive_Assert(smallest_in_open == find_smallest(open_root));
                    Assert(smallest_in_open->o_left == NULL);

                    smallest_in_open->o_left = neighbour;
                    neighbour->o_parent = smallest_in_open;
                    neighbour->list_status = Pathfinding_Node::IN_OPEN;

                    smallest_in_open = neighbour;
                    PF_Expensive_Assert(smallest_in_open == find_smallest(open_root));

                    PF_Assert_Tree_Valid(neighbour, neighbour->o_parent);
                    PF_Assert_Tree_Valid(open_root, NULL);
                }
                else
                {
                    // This branch is taken ~95% of the time (2021-02-24)
                    
                    Pathfinding_Node *parent = open_root;

                    Pathfinding_Node **parent_child_ptr = NULL;
                    while (true)
                    {
                        parent_child_ptr = (f_cost < f_costs[parent-nodes]) ? &parent->o_left : &parent->o_right;
                        if(*parent_child_ptr == NULL) break;
                        parent = *parent_child_ptr;
                    }
                    
                    neighbour->o_parent = parent;
                    *parent_child_ptr = neighbour;
                    neighbour->list_status = Pathfinding_Node::IN_OPEN;
                    
                    PF_Assert_Tree_Valid(neighbour, neighbour->o_parent);
                    PF_Assert_Tree_Valid(open_root, NULL);
                }



            }
        }
    }

    return success;
}


// NOTE: *_was_same_start_and_end_tile duration will only be set if allow_same_start_and_end_tile == false.
bool find_path_to_any(v3 p0, v3 *possible_p1s, int num_possible_p1s, Walk_Map *walk_map, bool do_reduce_path, Array<v3, ALLOC_TMP> *_path = NULL, double *_dur = NULL)
{
    Function_Profile();
    
    bool any_path_found = false;
    Array<v3, ALLOC_TMP> best_path = {0};
    double best_path_duration = DBL_MAX;
    
    Array<v3, ALLOC_TMP> path = {0};

    for(int i = 0; i < num_possible_p1s; i++)
    {
        path.n = 0;

        v3 p1 = possible_p1s[i];
    
        v3s start_tile = tile_from_p(p0);
        v3s end_tile   = tile_from_p(p1);

        if(start_tile == end_tile) {
            array_add_uninitialized(path, 2);
            path[0] = p0;
            path[1] = p1;
        }
        else if(!find_path(start_tile, end_tile, walk_map, false, &path)) {
            continue;
        }

        Assert(path.n != 1);
        
        double dur = player_walk_path_duration(path.e, path.n);
        if(dur < best_path_duration) {
            array_set(best_path, path);
            best_path_duration = dur;
            any_path_found = true;
        }
    }

    if(!any_path_found) return false;

    if(do_reduce_path) {
        remove_unnecessary_walk_path_nodes(&best_path);
        Assert(floats_equal(player_walk_path_duration(best_path.e, best_path.n), best_path_duration));
    }
    
    if(_dur)  *_dur = best_path_duration;
    if(_path) *_path = best_path;
    
    return true;
}
