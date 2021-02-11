
enum Layout_Type
{
    LAYOUT_AREA,
    LAYOUT_GRID
};

struct Area_Layout
{
    Rect a;
};

struct Grid_Layout
{
    Rect a;
    int cols;
    int rows;
    float spacing; // Spacing between cells
    float row_h;

    int current_cell;
};


struct Layout
{
    Layout_Type type;

    union {
        Area_Layout area;
        Grid_Layout grid;
    };
};

struct Layout_Manager
{
    v2 root_size; // TODO REMEMBER to set this to frame size every frame/build.
    Array<Layout, ALLOC_MALLOC> stack; // Make fixed-sized
};




Rect area(Layout_Manager *manager);
v2 center(Layout_Manager *manager);
