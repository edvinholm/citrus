
void reset_room_editor(Room_Editor *editor)
{
    editor->q = Q_IDENTITY;
    editor->z = 0;
    editor->selected_decor = DECOR_NONE_OR_NUM;
}

Tool_Stay_Open_Signal room_editor_ui(UI_Context ctx, Room_Editor *editor, bool is_current, double t, Client_UI *cui, double world_t, Room *room, Client *client)
{
    U(ctx);
    
    if(!is_current) return TOOL_CLOSE_INSTANTLY;

    _TOP_RIGHT_(240, 480);
    _WINDOW_(P(ctx), STRING("PALETTE"));
    _GRID_(2, 16, window_default_padding);

    for(int i = 0; i < DECOR_NONE_OR_NUM; i++) {
        _CELL_();
        bool selected = (editor->selected_decor == i);
        if(button(PC(ctx, i), EMPTY_STRING, true, selected) & CLICKED_ENABLED)
        {
            if(selected) editor->selected_decor = DECOR_NONE_OR_NUM;
            else         editor->selected_decor = (Decor_Type_ID)i;
        }
    }

    return TOOL_STAY_OPEN;
}

Entity decor_entity_preview(Room_Editor *editor, UI_World_View *wv, double world_t)
{
    Assert(editor->selected_decor != DECOR_NONE_OR_NUM);
    
    v3 tp = tp_from_index(wv->hovered_tile_ix);
        
    Entity e = {0};
    e.type = ENTITY_DECOR;
    e.decor.type = editor->selected_decor;
    e.decor.q    = editor->q;
    e.decor.p    = decor_entity_p_from_tp(tp, editor->selected_decor, e.decor.q);
    e.decor.p.z += editor->z;

    return e;
}

void room_editor_draw_in_world(Room_Editor *editor, UI_World_View *wv, double world_t, Room *room, Graphics *gfx)
{   
    if(editor->selected_decor != DECOR_NONE_OR_NUM) {
        auto e = decor_entity_preview(editor, wv, world_t);
        draw_entity(&e, world_t, gfx, room);
    }
}

void update_room_editor(Room_Editor *editor, UI_World_View *wv, double world_t, Room *room, Input_Manager *input, Client *client)
{
    if(editor->selected_decor == DECOR_NONE_OR_NUM) return;

    for(int k = 0; k < input->key_hits.n; k++) {
        switch(input->key_hits[k]) {
            case VKEY_z: {
                editor->q *= axis_rotation(V3_Z, TAU * .25f);
            } break;

            case VKEY_w: {
                editor->z += 1;
            } break;

            case VKEY_s: {
                editor->z -= 1;
            } break;

            case VKEY_e: {
                editor->z = 0;
            } break;
        }
    }
    
    if(wv->click_state & CLICKED_ENABLED) {
        auto e = decor_entity_preview(editor, wv, world_t);

        C_RS_Action c_rs_act = {0};
        c_rs_act.type = C_RS_ACT_DEVELOPER;
        c_rs_act.developer_packet.type = RSB_DEV_ADD_ENTITY;
        c_rs_act.developer_packet.add_entity.entity = e;
        
        array_add(client->connections.room_action_queue, c_rs_act);
    }
}
