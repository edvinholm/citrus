#include "view_user.cpp"
#include "view_calculator.cpp"

String title_for_view(View *view)
{
    switch(view->address.type) { // @Jai: #complete
        case VIEW_NONE:         return EMPTY_STRING;
        case VIEW_USER:         return title_for_user_view(view);
        case VIEW_CALCULATOR:   return title_for_calculator_view(view);
        default: Assert(false); return EMPTY_STRING;
    }
}

void post_view_open(View *view, Client *client)
{
    auto vtype = view->address.type;

    switch(vtype) { // @Jai: #complete
        case VIEW_CALCULATOR: post_calculator_view_open(view, client); break;
        default: break;
    }
}

void pre_view_close(View *view, Client *client)
{
    auto vtype = view->address.type;

    switch(vtype) { // @Jai: #complete
        case VIEW_CALCULATOR: pre_calculator_view_close(view, client); break;
        default: break;
    }
}


void replace_view(View *dest, View *src, Client *client)
{
    pre_view_close(dest, client);
    clear(dest);
    *dest = *src;
    post_view_open(dest, client);
}

void empty_view(UI_Context ctx, View *view, Input_Manager *input)
{
    U(ctx);
    
    auto *empty = &view->empty;
        
    { _TOP_(24);
        { _LEFT_CUT_(256);
            textfield(P(ctx), &empty->command_input, input);
        }
        { _LEFT_CUT_(64);
            if(button(P(ctx), STRING("OPEN")) & CLICKED_ENABLED) {
                View new_view;
                if(create_view_from_command(string(empty->command_input), ctx.client, &new_view)) {
                    replace_view(view, &new_view, ctx.client);
                }
            }
        }
    }

}

void view(UI_Context ctx, View *view, Input_Manager *input)
{
    U(ctx);

    auto vtype = view->address.type;

    switch(vtype) { // @Jai: #complete
        case VIEW_NONE: empty_view(P(ctx), view, input); break;
        case VIEW_USER: user_view(P(ctx), view); break;
        case VIEW_CALCULATOR: calculator_view(P(ctx), view); break;
    }
}
