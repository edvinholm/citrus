
String title_for_calculator_view(View *view)
{
    return STRING("CALCULATOR");
}


void calculator_view(UI_Context ctx, View *view)
{
    U(ctx);

    auto *util = &ctx.client->utilities;
    auto *calc_addr = &view->address.calculator;

    Rect a = area(ctx.layout);
    
    if(calc_addr->id < 0) {
        calc_addr->id = create_calculator(util);
        if(calc_addr->id < 0) {
            ui_text(P(ctx), STRING("Max number of calculators created"));
            return;
        }
    }
    
    auto *calc = get_calculator(calc_addr->id, util);
    Assert(calc);

    // DISPLAY //
    float display_h = min(50, 0.2 * a.h);
    { _TOP_CUT_(display_h);
        _PANEL_(P(ctx));
        float inner_h = area(ctx.layout).h;
        auto fs = round_font_size(inner_h);
        String display_text = concat_tmp("", calc->input);
        ui_text(P(ctx), display_text, fs, FONT_TITLE, HA_RIGHT, VA_CENTER);
    }

    // BUTTONS //
    { _GRID_(4, 5, 0);
        for(int y = 0; y < 5; y++) {
            for(int x = 0; x < 4; x++) {
                _CELL_();

                // DIGITS //
                if(x < 3 && y > 0 && y < 4) {
                    int digit = (2-y+1)*3+x+1;
                    if(button(PC(ctx, y*5+x), concat_tmp("", digit)) & CLICKED_ENABLED) {
                        enter_calculator_digit(digit, calc);
                    }
                }

                // OPERATORS //
                if(y == 0 && x == 1)
                    if(button(P(ctx), STRING("/")) & CLICKED_ENABLED)
                        enter_calculator_operator(CALC_OP_DIVIDE, calc);
                if(y == 0 && x == 2)
                    if(button(P(ctx), STRING("*")) & CLICKED_ENABLED)
                        enter_calculator_operator(CALC_OP_MULTIPLY, calc);
                if(y == 0 && x == 3)
                    if(button(P(ctx), STRING("-")) & CLICKED_ENABLED)
                        enter_calculator_operator(CALC_OP_SUBTRACT, calc);
                if(y == 1 && x == 3)
                    if(button(P(ctx), STRING("+")) & CLICKED_ENABLED)
                        enter_calculator_operator(CALC_OP_ADD, calc);

                // EXECUTE //
                if(y == 4 && x == 3)
                    if(button(P(ctx), STRING("=")) & CLICKED_ENABLED)
                        execute_calculator_operation(calc);
            }
        }
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
