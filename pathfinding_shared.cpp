

double player_walk_path_duration(v3 *path, u16 length)
{
    double t = 0;
    for(int i = 1; i < length; i++)
        t += magnitude(path[i] - path[i-1]) / player_walk_speed;
    
    return t;
}


void remove_unnecessary_walk_path_nodes(Array<v3, ALLOC_TMP> *path)
{
    for(int i = 1; i < path->n-1; i++)
    {
        v3 a = (*path)[i-1];
        v3 b = (*path)[i];
        v3 c = (*path)[i+1];
        
        v3 ab_dir = normalize(b - a);
        v3 bc_dir = normalize(c - b);

        if(is_zero(magnitude(bc_dir - ab_dir))) {
            array_ordered_remove((*path), i);
            i--;
        }
    }
}


// NOTE: Calling this function with start == end will result in return value = false.
// IMPORTANT: *_path needs to be in a valid state, and _path->n should be set to zero.
bool find_path(v3s start, v3s end, Walk_Mask *walk_map, Array<v3, ALLOC_TMP> *_path)
{
    Assert(_path->n == 0);

    if (start == end) return false;
    
    v2 end_p = { (float)end.x, (float)end.y };
    
    struct Node {
        int g_cost;
        int f_cost;
        u32 parent;
    };

    size_t nodes_size = sizeof(Node) * room_size_x * room_size_y;
    Node *nodes = (Node *)tmp_alloc(nodes_size);
    memset(nodes, 0, nodes_size);

    Array<s32, ALLOC_TMP> open   = {0};
    Array<s32, ALLOC_TMP> closed = {0};

    s32 start_ix = start.y * room_size_x + start.x;
    s32 end_ix   = end.y   * room_size_x + end.x;

    array_add(open, start_ix);

    while(true)
    {
        if(open.n == 0) return false;

        // Find the node with the lowest F cost in open.
        int current_ix_in_open;
        s32 current_ix;
        Node *current = NULL;
        for(int i = 0; i < open.n; i++)
        {
            auto ix = open[i];
            
            Assert(ix >= 0 && ix < room_size_x * room_size_y);
            Node *node = nodes + ix;

            Assert(i == 0 || current != NULL);
            if(i == 0 || node->f_cost < current->f_cost) {
                current_ix_in_open = i;
                current_ix = ix;
                current    = node;
            }
        }
        Assert(current);
        //--

        array_unordered_remove(open, current_ix_in_open);
        array_add(closed, current_ix);

        if(current_ix == end_ix) {
            // PATH FOUND //

            // Find length
            int path_length = 1;
            Node *n = current;
            while(true)
            {
                auto parent_ix = n->parent;
                path_length++;
                if(parent_ix == start_ix) break;
                n = &nodes[parent_ix];
            }

            array_add_uninitialized(*_path, path_length);
            
            v3 current_p = { (float)(current_ix % room_size_x), (float)(current_ix / room_size_x), 0 };
            (*_path)[path_length-1] = current_p;
            
            int path_node_ix = 1;
            n = current;
            while(true)
            {
                auto parent_ix = n->parent;
                v3 parent_p = { (float)(parent_ix % room_size_x), (float)(parent_ix / room_size_x), 0 };
                (*_path)[path_length-1-path_node_ix] = parent_p;
                
                if(parent_ix == start_ix) break;
                
                n = &nodes[parent_ix];
                path_node_ix++;
            }

            // Remove nodes that are not needed.
            remove_unnecessary_walk_path_nodes(_path);
            
            return true;
        }

        // The order of these is IMPORTANT.
        s32 neighbours[] = {
            current_ix - room_size_x, // North
            current_ix - 1,           // West
            current_ix + room_size_x, // South
            current_ix + 1,           // East
            
            current_ix - room_size_x - 1, // North-West
            current_ix + room_size_x - 1, // West-South
            current_ix + room_size_x + 1, // South-East
            current_ix - room_size_x + 1  // East-North
        };

        for(int i = 0; i < ARRLEN(neighbours); i++) {
            auto neighbour_ix = neighbours[i];
            if(neighbour_ix < 0 || neighbour_ix >= room_size_x * room_size_y) continue; // Outside the map.

            if(in_array(closed, neighbour_ix)) continue;

            Node *neighbour = &nodes[neighbour_ix];

            // TODO: Check if we can walk over the edge. Have a walk map with edges, or have disallowed edges as flags in the Walk_Mask.

            if(walk_map[neighbour_ix] & UNWALKABLE) continue;

            bool diagonal = (i >= 4);

            if(diagonal) {
                // Check that "neighbours" to the diagonal line are walkable.
                
                s32 a_ix = neighbours[(((i-4) % 4) + 4) % 4];
                s32 b_ix = neighbours[(((i-3) % 4) + 4) % 4];
                // These should always be inside the map if this diagonal neighbour is.
                Assert(a_ix >= 0 && a_ix < room_size_x * room_size_y);
                Assert(b_ix >= 0 && b_ix < room_size_x * room_size_y);

                if(walk_map[a_ix] & UNWALKABLE) continue;
                if(walk_map[b_ix] & UNWALKABLE) continue;
            }
            
            int rel_g_cost = (diagonal) ? 14 : 10;
            int g_cost = current->g_cost + rel_g_cost;

            v2 neighbour_p = { (float)(neighbour_ix % room_size_x), (float)(neighbour_ix / room_size_x) };
            int h_cost = roundf(magnitude(end_p - neighbour_p) * 10.0f);

            int f_cost = g_cost + h_cost;

            if(!in_array(open, neighbour_ix) || f_cost < neighbour->f_cost) {
                neighbour->g_cost = g_cost;
                neighbour->f_cost = f_cost;
                neighbour->parent = current_ix;
                ensure_in_array(open, neighbour_ix);
            }
        }
    }
}


// NOTE: *_was_same_start_and_end_tile duration will only be set if allow_same_start_and_end_tile == false.
bool find_path_to_any(v3 p0, v3 *possible_p1s, int num_possible_p1s, Walk_Mask *walk_map, Array<v3, ALLOC_TMP> *_path = NULL, double *_dur = NULL)
{
    /*
    Assert(e->type == ENTITY_PLAYER);
    auto *player_e = &e->player_e;

    v3 p0 = entity_position(e, room->t, room);
    */
    
    bool any_path_found = false;
    Array<v3, ALLOC_TMP> best_path = {0};
    double best_path_duration = DBL_MAX;
    
    Array<v3, ALLOC_TMP> path = {0};

    for(int i = 0; i < num_possible_p1s; i++)
    {
        path.n = 0;

        v3 p1 = possible_p1s[i];
    
        v3s start_tile = { (s32)roundf(p0.x), (s32)roundf(p0.y), (s32)roundf(p0.z) };
        v3s end_tile   = { (s32)roundf(p1.x), (s32)roundf(p1.y), (s32)roundf(p1.z) };

        if(start_tile == end_tile) {
            array_add_uninitialized(path, 2);
            path[0] = p0;
            path[1] = p1;
        }
        else if(!find_path(start_tile, end_tile, walk_map, &path)) {
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
    
    if(_dur)  *_dur = best_path_duration;
    if(_path) *_path = best_path;
    
    return true;
}
