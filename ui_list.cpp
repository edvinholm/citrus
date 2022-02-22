

void list_entry(UI_Context ctx, float *column_width_percentages, int num_columns, List_Entry_Cell *cells)
{
    _LIST_({"Name", .4}, {"Age", .1}, {"Location", .2}, {"Actions", .3});
    for() {
        _LIST_ENTRY_();
        list_cell(STRING("Adolf"));
        { _LIST_CELL_();
            button(PC(ctx, i), STRING("5"));
        }
            
    }
    
}
