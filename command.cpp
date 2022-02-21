

String view_type_command_identifiers[] = {
    STRING("EMPTY"),
    STRING("USER"),
    STRING("CALC"),
    STRING("WORLD")
};
static_assert(ARRLEN(view_type_command_identifiers) == VIEW_NONE_OR_NUM);


// @Incomplete: We ignore parameters here.
String command_from_view_address(View_Address addr)
{
    return view_type_command_identifiers[addr.type];
}


// NOTE: Will populate *addr_. addr_ should contain the already parsed view type.
void set_default_view_address_parameters(View_Address *addr_)
{
    switch(addr_->type) {
        case VIEW_USER:       addr_->user.id = 77; break;
        case VIEW_CALCULATOR: addr_->calculator.id = -1; break;
    }
}

// NOTE: Will populate *addr_. addr_ should contain the already parsed view type.
bool parse_view_address_command_after_type_identifier(String command, View_Address *addr_, Client *client)
{
    switch(addr_->type) { // @Jai: #complete

        case VIEW_CALCULATOR: {
            u32 id;
            if(is_only_digits(command) && parse_u32(command, &id) &&
               get_calculator(id, &client->utilities))
                addr_->calculator.id = id;
            else
                return false;
            return true;
        } break;
        
        default: return false;
    }
}


bool view_address_from_command(String command, Client *client, View_Address *_addr)
{
    View_Address addr = {0};
    
    for(int i = 0; i < ARRLEN(view_type_command_identifiers); i++) {
        auto &ident = view_type_command_identifiers[i];
        if(starts_with(command, ident)) {
            addr.type = (View_Type)i;

            set_default_view_address_parameters(&addr);
            
            auto sub = sub_string(command, ident.length);
            if(sub.length > 0 && sub[0] == ' ') {
                auto post = sub_string(sub, 1);
                if(!parse_view_address_command_after_type_identifier(post, &addr, client)) return false;
            }
            
            *_addr = addr;
            return true;
        }
    }

    return false;
}
