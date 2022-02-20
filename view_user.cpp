
String title_for_user_view(View *view)
{
    return concat_tmp("USER #", view->address.user.id);
}

void user_view(UI_Context ctx, View *view)
{
    U(ctx);
    button(P(ctx), STRING("USER"));
}
