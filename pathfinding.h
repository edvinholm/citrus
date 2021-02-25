
static_assert(room_size_x * room_size_y < U16_MAX); // Because we use u16:s to reference tiles in pathfinding. < and not <= because we use U16_MAX as no tile..

enum Walk_Flag_
{
    UNWALKABLE = 0x0001
};

typedef u8 Walk_Flags;

struct Walk_Map_Node
{
    Walk_Flags flags;
};

struct Pathfinding_Node
{
    // Set by pathfinding, depending on start and end of path //
    enum Pathfinding_List_Status: u8 {
        IN_NO_LIST = 0,
        IN_OPEN,
        IN_CLOSED
    };
    Pathfinding_List_Status list_status;
    
    u16 came_from;

    // Open List Binary Tree:
    Pathfinding_Node *o_parent; 
    Pathfinding_Node *o_left;
    Pathfinding_Node *o_right;
    // // //
};

struct Walk_Map
{
    Walk_Map_Node nodes[room_size_x * room_size_y];
    
};


