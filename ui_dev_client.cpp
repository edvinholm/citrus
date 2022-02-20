
void init_developer_ui(Developer_UI *dui)
{
    init_ui_dock(&dui->dock);
}


void client_developer_window(UI_Context ctx, Input_Manager *input, Developer_UI *dui, Client *client);
void set_current_tool(Tool_ID tool, Client_UI *cui, double t);

double dev_ui_error_indication_factor(double t, Client *client)
{
    double result = 0;
    
    auto &pendings = client->room.pending_rs_operations;
    for(int i = 0; i < pendings.n; i++) {
        
        auto *pending = &pendings[i];
        auto dt = (t - pending->creation_t);
        result = max(result, clamp((dt - .5) / 2.5));
    }
    
    return result;
}

void client_developer_window(UI_Context ctx, double t, Input_Manager *input, Developer_UI *dui, Client *client)
{
    U(ctx);
    
    _WINDOW_(P(ctx), STRING("DEVELOPER"));

    ui_dock(P(ctx), &dui->dock, input);
    
    { _TOP_CUT_(32);
        bool selected = dui->sprite_editor.open;
        if(button(P(ctx), STRING("SPRITES"), true, selected) & CLICKED_ENABLED) {
            dui->sprite_editor.open = !dui->sprite_editor.open;
        }
    }

    auto &pendings = client->room.pending_rs_operations;
    for(int i = 0; i < pendings.n; i++) {
        auto *pending = &pendings[i];
        _TOP_SLIDE_(20);

        char *received_str = (pending->result_received) ? "RECEIVED" : "NOT RECEIVED";
        char *success_str  = "???";
        if(pending->result_received)
            success_str = (pending->success) ? "SUCCESS" : "FAILURE";

        double dt = (t - pending->creation_t);
        
        v4 color = C_BLACK;
        color = lerp(color, C_RED, clamp((dt - .5) / 2.5));
        
        String txt = concat_tmp("RSB: ", pending->rsb_packet_id, " | ", received_str, " | ", success_str, " | ", dt);
        ui_text(PC(ctx, i), txt, FS_14, FONT_BODY, HA_LEFT, VA_CENTER, opt(color));
    }
}


void client_developer_ui(UI_Context ctx, double t, Input_Manager *input, Developer_UI *dui, Client *client)
{
    U(ctx);

    if(dui->sprite_editor.open) {
        sprite_editor(P(ctx), input, &dui->sprite_editor);
    }
    
    if(dui->main_window_open) {
        _RIGHT_(320); _BOTTOM_(480);        
        client_developer_window(P(ctx), t, input, dui, client);
    }

}
