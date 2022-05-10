#include "view_user.cpp"
#include "view_people.cpp"
#include "view_calculator.cpp"

#include "view_dev_servers.cpp"

String title_for_view(View *view)
{
    switch(view->address.type) { // @Jai: #complete
        case VIEW_EMPTY:        return EMPTY_STRING;
        case VIEW_USER:         return title_for_user_view(view);
        case VIEW_CALCULATOR:   return title_for_calculator_view(view);
        case VIEW_WORLD:        return STRING("WORLD"); 
        case VIEW_PEOPLE:       return STRING("PEOPLE");
    }
    
    return EMPTY_STRING;
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


void replace_view(View *dest, View *src, Client *client, UI_Dock *dock = NULL)
{
    pre_view_close(dest, client);
    clear(dest);
    *dest = *src;
    post_view_open(dest, client);

    if(dock) dock->unsaved_changes = true;
}

void empty_view(UI_Context ctx, View *view, Input_Manager *input, UI_Dock *dock = NULL)
{
    U(ctx);
    
    auto *empty = &view->empty;

    Rect a = area(ctx.layout);
        
    // COMMAND INPUT //
    float cmd_w = max(128, min(a.w - 16, 256));
    { _BOTTOM_(24); _LEFT_(cmd_w); _TRANSLATE_XY_(a.w * .5f - cmd_w * .5f, a.h * .5f - 24 * .5f);
        
        { _RIGHT_CUT_(64);
            if(button(P(ctx), STRING("OPEN")) & CLICKED_ENABLED) {
                View new_view;
                Zero(new_view);
                if(view_address_from_command(string(empty->command_input), ctx.client, &new_view.address)) {
                    replace_view(view, &new_view, ctx.client, dock);
                }
            }
        }
        
        textfield(P(ctx), &empty->command_input, input);
    }

}

void view(UI_Context ctx, View *view, Input_Manager *input, UI_Dock *dock = NULL)
{
    U(ctx);

    auto vtype = view->address.type;

    switch(vtype) { // @Jai: #complete
        case VIEW_EMPTY: empty_view(P(ctx), view, input, dock); break;
        case VIEW_USER: user_view(P(ctx), view); break;
        case VIEW_CALCULATOR: calculator_view(P(ctx), view); break;
        case VIEW_WORLD: world_view(P(ctx)); break;
        case VIEW_PEOPLE: people_view(P(ctx)); break;
            
        case VIEW_DEV_SERVERS: dev_servers_view(P(ctx)); break;
    }
}
