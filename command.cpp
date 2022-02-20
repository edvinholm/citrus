


bool create_view_from_command(String command, Client *client, View *_view)
{
    View view;
    Zero(view);
    
    if(equal(command, "USER")) {
        view.address.type = VIEW_USER;
        view.address.user.id = 77;
    } else if(starts_with(command, "CALC")) {
        view.address.type = VIEW_CALCULATOR;
        view.address.calculator.id = -1;

        auto sub = sub_string(command, 4, command.length);
        if(sub.length > 0) {
            if(sub[0] == ' ') {
                sub = sub_string(sub, 1, sub.length);
                u32 id;
                if(is_only_digits(sub) && parse_u32(sub, &id) &&
                   get_calculator(id, &client->utilities))
                    view.address.calculator.id = id;
                else
                    return false;
            } else return false;
        }
        
    } else {
        return false;
    }

    *_view = view;
    return true;
}
