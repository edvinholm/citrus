

void foo(UI_Manager *ui)
{
    push_ui_location(__COUNTER__+1, 32, ui);
    pop_ui_location(ui);
}


void baz(UI_Context ctx)
{
    UNPACK(ctx);
    
    Debug_Print("ctx state is %d\n", ctx.get_state());

    button(PACK(ctx));
    button(PACK(ctx)); button(PACK(ctx));
    button(PACK(ctx));

    Debug_Print("LOOP:\n");
    for(int i = 0; i < 5; i++)
    {
        button(PACK_COUNT(ctx, i));
        button(PACK_COUNT(ctx, i));
    }
}

void bar(UI_Context ctx)
{
    UNPACK(ctx);

    button(PACK(ctx));
    
    Debug_Print("ctx state is %d\n", ctx.get_state());
    baz(PACK(ctx));
    
    button(PACK(ctx));
}
