
void init_developer_ui(Developer_UI *dui)
{
    init_ui_dock(&dui->dock);
}


void client_developer_window(UI_Context ctx, Input_Manager *input, Developer_UI *dui, Client *client)
{
    U(ctx);
    
    _WINDOW_(P(ctx), STRING("DEVELOPER"));

    ui_dock(P(ctx), &dui->dock, input);
}
