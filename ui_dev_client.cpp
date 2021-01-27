

void client_developer_window(UI_Context ctx, Input_Manager *input, Client *client)
{
    U(ctx);
    
    _WINDOW_(P(ctx), STRING("DEV TOOLS"));

    if(button(P(ctx)) & CLICKED_ENABLED) Assert(false);
}
