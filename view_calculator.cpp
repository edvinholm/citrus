
String title_for_calculator_view(View *view)
{
    return concat_tmp("CALCULATOR #", view->address.calculator.id);
}


void calculator_view(UI_Context ctx, View *view)
{
    U(ctx);

    auto *util = &ctx.client->utilities;
    auto *calc_addr = &view->address.calculator;
    
    if(calc_addr->id < 0) {
        calc_addr->id = create_calculator(util);
        if(calc_addr->id < 0) {
            ui_text(P(ctx), STRING("Max number of calculators created"));
            return;
        }
    }
    
    auto *calc = get_calculator(calc_addr->id, util);
    Assert(calc);
    
    { _TOP_(32);
        ui_text(P(ctx), concat_tmp("", calc->input));
        panel(P(ctx));
    }
        
}


void post_calculator_view_open(View *view, Client *client) {
    auto *util = &client->utilities;
    auto *calc_addr = &view->address.calculator;

    if(calc_addr->id < 0) return;

    modify_calculator_users(calc_addr->id, true, util);
}


void pre_calculator_view_close(View *view, Client *client) {
    auto *util = &client->utilities;
    auto *calc_addr = &view->address.calculator;

    if(calc_addr->id < 0) return;

    modify_calculator_users(calc_addr->id, false, util);
}
