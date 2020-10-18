
void push_layout(Layout layout, Layout_Manager *manager)
{
    array_add(manager->stack, layout);
}

void pop_layout(Layout_Manager *manager)
{
    Assert(manager->stack.n > 0);
    manager->stack.n--;
}

Layout *current_layout(Layout_Manager *manager)
{
    return (manager->stack.n > 0) ? last_element_pointer(manager->stack) : NULL;
}



Rect area(Layout_Manager *manager);
v2 center(Layout_Manager *manager);

#include "layout_area.cpp"
#include "layout_grid.cpp"


Rect area(Layout_Manager *manager)
{
    if(manager->stack.n == 0)
        return rect(V2_ZERO, manager->root_size);

    Layout *layout = current_layout(manager);
    switch(layout->type)
    {
        case LAYOUT_AREA:
            return layout->area.a;

        case LAYOUT_GRID:
            return current_area_from_grid_layout(&layout->grid);
    }
    
    Assert(false);
    return {0};
}

inline
v2 center(Layout_Manager *manager)
{
    return center_of(area(manager));
}




