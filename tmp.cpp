

void foo(UI_Manager *ui)
{
    push_ui_location(__COUNTER__+1, 32, ui);
    pop_ui_location(ui);
}


void baz(UI_Context ctx)
{
    U(ctx);

    button(P(ctx));
    button(P(ctx)); button(P(ctx));
    button(P(ctx));

    Debug_Print("LOOP:\n");
    for(int i = 0; i < 5; i++)
    {
        button(PC(ctx, i));
        button(PC(ctx, i));
    }
}

void bar(UI_Context ctx)
{
    U(ctx);

    button(P(ctx));
    
    baz(P(ctx));
    
    button(P(ctx));
}
