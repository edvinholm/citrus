
struct Client
{
    UI_Manager ui;
};


#include "tmp.cpp"

int client_entry_point(int num_args, char **arguments)
{
    Debug_Print("I am a client.\n");
    
    Client client = {0};
    UI_Manager *ui = &client.ui;

#if 0
    for(int i = 0; i < 3; i++)
    {
        push_ui_location(__COUNTER__+1, 1, ui);
        push_ui_location(__COUNTER__+1, 1, ui);
        foo(ui);
        push_ui_location(__COUNTER__+1, 1, ui);
        push_ui_location(__COUNTER__+1, 1, ui);
        push_ui_location(__COUNTER__+1, 1, ui);
        
        pop_ui_location(ui);
        pop_ui_location(ui);
        pop_ui_location(ui);
        pop_ui_location(ui);
        pop_ui_location(ui);
    }
#else
    UI_Context ctx = UI_Context();
    ctx.manager = ui;

    /*
    UI_Context ctx_a = UI_Context();
    pack(&ctx, &ctx_a, __COUNTER__+1, 0);
    */

    Debug_Print("bar(PACK):\n");
    bar(PACK(ctx));
    
    //auto ctx_b = pack(&ctx, __COUNTER__+1, 0);
    //bar(ctx_b);
    
#endif
 
    return 0;
}
