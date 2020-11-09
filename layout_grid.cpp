

Rect grid_cell(int col, int row, Rect grid_a, int cols, int rows, float spacing, float row_h = 0.0f)
{
    if(is_zero(row_h))
        row_h = (grid_a.h - (rows-1) * spacing)/rows;
    
    v2 s = { (grid_a.w - (cols-1) * spacing)/cols, row_h };
    v2 cell_p_rel = { (float)col * (s.w + spacing),
                      (float)row * (s.h + spacing) };
    return rect(grid_a.p + cell_p_rel, s);
}

Rect grid_cell(int cell_index, Rect grid_a, int cols, int rows, float spacing)
{
    return grid_cell(cell_index % cols, cell_index % rows, grid_a, cols, rows, spacing);
}

Rect grid_cell(int col, int row, Grid_Layout *grid)
{
    Assert(col >= 0 && col < grid->cols);
    Assert(row >= 0 && row < grid->rows);

    return grid_cell(col, row, grid->a, grid->cols, grid->rows, grid->spacing, grid->row_h);
}

Rect grid_cell(int cell_index, Grid_Layout *grid)
{
    return grid_cell(cell_index % grid->cols, cell_index / grid->cols, grid);
}


void push_grid_layout(Rect a, int cols, int rows, float spacing, Layout_Manager *manager, float row_h = 0.0f)
{
    Layout layout;
    Zero(layout);
    layout.type = LAYOUT_GRID;

    Grid_Layout &grid = layout.grid;
    grid.a = a;
    grid.cols = cols;
    grid.rows = rows;
    grid.spacing = spacing;
    grid.current_cell = -1;
    grid.row_h = row_h;

    push_layout(layout, manager);
}

void push_grid_layout(int cols, int rows, float spacing, Layout_Manager *manager, float row_h = 0.0f)
{
    push_grid_layout(area(manager), cols, rows, spacing, manager, row_h);
}

Grid_Layout *current_grid_layout(Layout_Manager *manager)
{
    Layout *layout = current_layout(manager);
    Assert(layout->type == LAYOUT_GRID);
    
    return &layout->grid;
}

Rect grid_cell(int col, int row, Layout_Manager *manager)
{
    return grid_cell(col, row, current_grid_layout(manager));
}

Rect set_grid_cell(int cell_index, Layout_Manager *manager, Grid_Layout *grid = NULL)
{
    if(!grid)
        grid = current_grid_layout(manager);

    grid->current_cell = cell_index;
        Assert(grid->current_cell >= 0 && grid->current_cell < grid->rows * grid->cols);

    return grid_cell(grid->current_cell, grid);
}

Rect next_grid_cell(Layout_Manager *manager)
{
    Grid_Layout *grid = current_grid_layout(manager);
    return set_grid_cell(grid->current_cell + 1, manager, grid);
}


////////////



/////////////



inline
Rect current_area_from_grid_layout(Grid_Layout *grid)
{    
    if(grid->current_cell < 0)
        return grid->a;

    return grid_cell(grid->current_cell, grid);
}






/////////////// ////////////////////// /////////////////////
/////////////// ////////////////////// /////////////////////
/////////////// ////////////////////// /////////////////////
/////////////// ////////////////////// /////////////////////


#define _GRID_(...) \
    push_grid_layout(__VA_ARGS__, ctx.layout);             \
    defer(pop_layout(ctx.layout););


#define _CELL_() \
    next_grid_cell(ctx.layout);


inline
void push_columns(int n, Layout_Manager *manager, float spacing = 0.0f) { push_grid_layout(area(manager), n, 1, spacing, manager); }

#define _COLUMNS_(...) \
    push_columns(__VA_ARGS__, ctx.layout);                 \
    defer(pop_layout(ctx.layout););

#define _COL_() \
    _AREA_(next_grid_cell(ctx.layout));

#define _COL_I_(Ix) \
    _AREA_(grid_cell(Ix, 0, current_grid_layout(ctx.layout)));


inline
void push_rows(int n, Layout_Manager *manager, float spacing = 0.0f) { push_grid_layout(area(manager), 1, n, spacing, manager); }

#define _ROWS_(...) \
    push_rows(__VA_ARGS__, ctx.layout);                    \
    defer(pop_layout(ctx.layout););

#define _ROW_() \
    _AREA_(next_grid_cell(ctx.layout));

#define _ROW_I_(Ix) \
    _AREA_(grid_cell(0, Ix, current_grid_layout(ctx.layout)));

