
void begin_list(UI_Context ctx, String *column_titles, float *column_width_fractions, int num_columns,
                float row_h = 24)
{
    U(ctx);

    push_grid_layout(num_columns, 1, 0, ctx.layout, row_h, column_width_fractions);
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

void end_list_cell(UI_Context ctx)
{
    panel(P(ctx));
}

void list_cell(UI_Context ctx, String text)
{
    U(ctx);
    
    begin_list_cell(P(ctx));
    ui_text(P(ctx), text);
    end_list_cell(P(ctx));
    //_LIST_CELL_();
}

