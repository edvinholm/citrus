
void begin_list(UI_Context ctx, String *column_titles, float *column_width_fractions, int num_columns,
                float row_h = 24)
{
    U(ctx);

    push_grid_layout(num_columns, -1, 0, ctx.layout, row_h, column_width_fractions);
    for(int i = 0; i < num_columns; i++) {
        _CELL_();
//        if(i != 2) continue;
        button(PC(ctx, i), column_titles[i]);
    }
}

void end_list(UI_Context ctx)
{
    U(ctx);
    
    pop_layout(ctx.layout);
}

void begin_list_cell(UI_Context ctx)
{
    U(ctx);
    next_grid_cell(ctx.layout);
}

UI_Click_State end_list_cell(UI_Context ctx)
{
    U(ctx);

    auto *e = panel(P(ctx), {0}, UI_PANEL_STYLE_LIST_CELL);
    e->clickthrough = true;
    
    Grid_Layout *grid = current_grid_layout(ctx.layout);
    if((grid->current_cell+1) % grid->cols != 0) return IDLE;
    
    Rect aa = area(ctx.layout);

    aa.x = grid->a.x;
    aa.w = grid->a.w;

    _AREA_(aa);
    return button(P(ctx), EMPTY_STRING, true, false, {0}, UI_BUTTON_STYLE_LIST_ITEM);
}

void list_cell(UI_Context ctx, String text)
{
    U(ctx);
    
    begin_list_cell(P(ctx));
    { _SHRINK_(8, 8, 0, 0);
        ui_text(P(ctx), text, FS_14, FONT_BODY, HA_LEFT, VA_CENTER);
    }
    end_list_cell(P(ctx));
    //_LIST_CELL_();
}

