

void request_connection_to_room(Room_ID id, Client *client)
{
    Assert(client->server_connections.room_connect_requested == false);
    
    client->server_connections.requested_room = id;
    client->server_connections.room_connect_requested = true;
}

void request_connection_to_user(User_ID user_id, Client *client)
{
    Assert(client->server_connections.user_connect_requested == false);

    client->server_connections.requested_user = user_id;
    client->server_connections.user_connect_requested = true;
}

void user_window(UI_Context ctx, Client *client)
{
    U(ctx);
    
    char *usernames[] = {
        "Tachophobia",
        "Sailor88",
        //"WhoLetTheDogsOut",
        "MrCool",
        "kadlfgAJb!",
        "LongLongWay.9000",
        "_u_s_e_r_n_a_m_e_",
        "generalW4ste",
        "Snordolf101"
    };

    User_ID user_ids[] = {
        1,
        2,
        //3,
        4,
        5,
        6,
        7,
        8,
        9
    };
    static_assert(ARRLEN(usernames) == ARRLEN(user_ids));

    auto *user = &client->user;
    
    bool connected  = client->server_connections.user.status == USER_CONNECT__CONNECTED; // @Cleanup
    bool connecting = client->server_connections.user_connect_requested;

    _WINDOW_(P(ctx), user->username, true, connected, user->color);

    if(connected) {
        _TOP_CUT_(480 + 72);
        cut_bottom(4, ctx.layout);

        { _BOTTOM_CUT_(72);
            ui_text(P(ctx), concat_tmp("Money: ¤", user->money, ctx.manager->string_builder));
        }

        int cols = 8;
        int rows = 14;
        _GRID_(cols, rows, 2);
        for(int r = 0; r < rows; r++) {
            for(int c = 0; c < cols; c++) {
                _CELL_();

                auto cell_ix = r * cols + c;
                Item *item = &user->inventory[cell_ix];
                if(item->id == NO_ITEM) item = NULL;

                bool enabled = item != NULL;

                bool selected = (cell_ix == user->selected_inventory_item_plus_one - 1);
                if(ui_inventory_slot(PC(ctx, cell_ix), item, enabled, selected) & CLICKED_ENABLED) {
                    // @Norelease @Robustness: Make this an item ID.
                    //   So if the gets removed or moved on the server, the item will be
                    //   deselected, and not replaced by another item that takes its slot
                    //   in the inventory.
                    user->selected_inventory_item_plus_one = cell_ix + 1;
                }
            }
        }
    }
    
    
    auto *us_con = &client->server_connections.user;
    User_ID current_user = us_con->current_user;

    _GRID_(1, ARRLEN(usernames), 4);
    for(int i = 0; i < ARRLEN(usernames); i++)
    {
        _CELL_();
        
        String  username = STRING(usernames[i]);
        User_ID id       = user_ids[i];
        bool is_current  = (id == current_user);
        
        if(button(PC(ctx, i), username, !connecting, is_current) & CLICKED_ENABLED)
        {
            request_connection_to_user(id, client);
        }
    }
}

User_ID current_user_id(Client *client)
{
    if(client->server_connections.user.status != USER_SERVER_CONNECTED) return NO_USER;
    if(!client->user.initialized) return NO_USER;

    Assert(client->user.id == client->server_connections.user.current_user);
    return client->user.id;
}

User *current_user(Client *client)
{
    if(current_user_id(client) == NO_USER) return NULL;
    return &client->user;
}

bool action_of_type_queued(C_RS_Action_Type type, Client *client)
{
    auto &queue = client->server_connections.room_action_queue;
    for(int i = 0; i < queue.n; i++) {
        if(queue[i].type == type) return true;
    }

    return false;
}

String entity_action_label(Entity_Action action)
{
    switch(action.type) {
        case ENTITY_ACT_PICK_UP: return STRING("PICK UP"); break;
        case ENTITY_ACT_HARVEST: return STRING("HARVEST"); break;
        case ENTITY_ACT_SET_POWER_MODE: {
            auto *x = &action.set_power_mode;
            if(x->set_to_on) {
                return STRING("START");
            } else {
                return STRING("STOP");
            }
        } break;

        default: Assert(false); return STRING("???"); break;
    }

    Assert(false);
    return EMPTY_STRING;
}

// NOTE: If the item is on an entity, REMEMBER to do update_entity_item()!
// REMEMBER: world_t is local to the Room Server we're on. User Server has no world_t.
//           So passing a world_t but no entity does not make sense... -EH, 2021-01-30
void item_window(UI_Context ctx, Item *item, Client *client, UI_Click_State *_close_button_state = NULL, Entity *e = NULL, double world_t = 0)
{
    U(ctx);
    
    Assert(item != NULL);
    Assert(item->type != ITEM_NONE_OR_NUM);

    auto *type = &item_types[item->type];

    UI_ID window_id;
    Rect window_a = begin_window(&window_id, P(ctx), type->name);
    {
        _AREA_(window_a);

        if(e) {
            Assert(e->type == ENTITY_ITEM);
            
            Array<Entity_Action_Type, ALLOC_TMP> actions = {0};
            get_available_actions_for_entity(e, &actions);

            for(int i = 0; i < actions.n; i++) {

                if(i > 0) { _BOTTOM_CUT_(window_default_padding); }
                
                _BOTTOM_CUT_(32);
                
                Entity_Action entity_action = {0};
                entity_action.type = actions[i];

                // @Cleanup: This should be done somewhere else...
                //           Should get_available_actions_for_entity return Entity_Actions instead of Entity_Action_Types?
                //           But some actions we want the user to decide parameters for.
                switch(entity_action.type) {
                    case ENTITY_ACT_SET_POWER_MODE: {
                        Assert(e->item_e.item.type == ITEM_MACHINE);
                        auto *machine_e = &e->item_e.machine;
                        entity_action.set_power_mode.set_to_on = (machine_e->stop_t >= machine_e->start_t);
                    } break;
                }
    
                bool enabled = entity_action_predicted_possible(entity_action, e, client->user.id, world_t, &client->user);
                
                if(button(PC(ctx, i), entity_action_label(entity_action), enabled) & CLICKED_ENABLED) {

                    C_RS_Action action = {0};
                    action.type = C_RS_ACT_ENTITY_ACTION;                    
                    auto &act = action.entity_action;

                    act.entity = e->id;
                    act.action = entity_action;

                    array_add(client->server_connections.room_action_queue, action);
                }
            }
        }

        auto image_s = area(ctx.layout).w * (1 - 0.61803);
        { _TOP_CUT_(image_s);
        
            { _LEFT_CUT_(image_s);
                button(P(ctx));
            }

            _SHRINK_(window_default_padding);

            { _TOP_CUT_(20);
                v3s vol = type->volume;
                String volume_str = concat_tmp("Dimensions: ", vol.x, "x", vol.y, "x", vol.z, ctx.manager->string_builder);
                ui_text(P(ctx), volume_str);
            }

#if DEBUG
            { _TOP_CUT_(20);
                String id_str = concat_tmp("ID: ", item->id.origin, ":", item->id.number, ctx.manager->string_builder);
                ui_text(P(ctx), id_str);
            }
#endif
        }

        switch(item->type) {
            case ITEM_PLANT: {
                auto *plant = &item->plant;
                    
                { _TOP_CUT_(20);   
                    float grow_progress = plant->grow_progress;
                    String grow_str = concat_tmp("Grow progress: ", (int)(grow_progress * 100.0f), "%", ctx.manager->string_builder);
                    ui_text(P(ctx), grow_str);
                }
            };
        }
    }
    end_window(window_id, ctx.manager, _close_button_state);
}

void room_window(UI_Context ctx, Client *client)
{
    U(ctx);

    float room_button_h = 64;

    // @Temporary
    String_Builder sb = {0};
    
    const int num_rooms = 15;

    Room_ID requested_room = (client->server_connections.room_connect_requested) ? client->server_connections.requested_room : -1;
    Room_ID current_room   = client->server_connections.room.current_room;

    // TODO @Cleanup: :PushPop Window
    { _WINDOW_(P(ctx), STRING("ROOM"));
        { _TOP_CUT_(room_button_h);
            _GRID_(num_rooms, 1, window_default_padding);
            for(int r = 0; r < num_rooms; r++)
            {
                _CELL_();
                bool selected;
                if(requested_room != -1) selected = (requested_room == r);
                else                     selected = (current_room   == r);

                if(button(PC(ctx, r), concat_tmp("", r, sb), (requested_room == -1), selected) & CLICKED_ENABLED)
                {
                    request_connection_to_room((Room_ID)r, client);
                }
            }
        }
    }
}


void chat_panel(UI_Context ctx, Input_Manager *input, Client *client)
{
    U(ctx);

    auto *user = current_user(client);
    if(!user) return;
    
    auto *draft = &client->user.chat_draft;
            
    { _SHRINK_(4);
        { _LEFT_CUT_(480);
            bool enabled = true;
            bool text_did_change;

            String text = { draft->e, draft->n };
            text = textfield_tmp(text, input, P(ctx), &text_did_change, enabled);
            array_set(*draft, text.data, text.length);
        }
        cut_left(4, ctx.layout);

        { _LEFT_SQUARE_CUT_();

            bool enabled = (!action_of_type_queued(C_RS_ACT_CHAT, client) &&
                            draft->n > 0);
            
            if(button(P(ctx), STRING("SAY"), enabled) & CLICKED_ENABLED)
            {
                C_RS_Action action = {0};
                action.type = C_RS_ACT_CHAT;
                auto &chat = action.chat;

                array_add(client->server_connections.room_action_queue, action); // @Norelease: IMPORTANT: Any time we want to enqueue a C_RS_Action, we should check that we're connected to a room!
            }
        }
    }
    
    panel(P(ctx));
}

void client_ui(UI_Context ctx, Input_Manager *input, double t, Client *client)
{
    U(ctx);

    Client_UI *cui = &client->cui;

    Rect a = area(ctx.layout);

    auto *room = &client->game.room;
    auto world_t = world_time_for_room(room, t);

    Entity *player_entity = NULL;
    User_ID current_user = current_user_id(client);
    if(current_user != NO_USER) {
        player_entity = find_player_entity(current_user, room);
    }

    const float menu_bar_button_s = 64;
    
    { _LEFT_CUT_(menu_bar_button_s);

        { _AREA_COPY_();
        
            { _TOP_SLIDE_(menu_bar_button_s);
                if(button(P(ctx), STRING("ROOM"), true, cui->room_window_open) & CLICKED_ENABLED) {
                    cui->room_window_open = !cui->room_window_open;
                }
            }
            
            { _TOP_SLIDE_(menu_bar_button_s);
                if(button(P(ctx), STRING("USER"), true, cui->user_window_open) & CLICKED_ENABLED) {
                    cui->user_window_open = !cui->user_window_open;
                }
            }

#if DEVELOPER
            { _TOP_SLIDE_(menu_bar_button_s);
                if(button(P(ctx), STRING("DEV"), true, cui->dev_window_open) & CLICKED_ENABLED) {
                    cui->dev_window_open = !cui->dev_window_open;
                }
            }
#endif
        }

        panel(P(ctx));
    }
    

    { _RIGHT_CUT_(64);
        
    }

        
#if DEVELOPER
    if(cui->dev_window_open)
    { _RIGHT_(640); _BOTTOM_(480);
        client_developer_window(P(ctx), input, client);
    }
#endif


    if(cui->room_window_open)
    { _TOP_(window_border_width + window_title_height + window_default_padding + 64 + window_default_padding + window_border_width);
        room_window(P(ctx), client);
    }

    if(cui->user_window_open)
    { _RIGHT_CUT_(320);
        user_window(P(ctx), client);
    }

    
    { _SHRINK_(16); _LEFT_(240); _BOTTOM_(360);
        
        Item *selected_item = get_selected_inventory_item(&client->user);
        if(selected_item) {
            item_window(P(ctx), selected_item, client);
        }

        
        if(room->selected_entity != NO_ENTITY)
        {
            Entity *e = find_entity(room->selected_entity, room);
            if(e) {
                if(e->type == ENTITY_ITEM)
                {
                    update_entity_item(e, world_t);

                    UI_Click_State close_button_state;
                    item_window(P(ctx), &e->item_e.item, client, &close_button_state, e, world_t);
                    if(close_button_state & CLICKED_ENABLED) {
                        room->selected_entity = NO_ENTITY;
                    }
                }
            }
        }
    }

    // BOTTOM PANEL //
    if(cui->open_bottom_panel_tab != BP_TAB_NONE_OR_NUM)
    { _BOTTOM_CUT_(100);

        switch(cui->open_bottom_panel_tab) {
            case BP_TAB_CHAT: {
                chat_panel(P(ctx), input, client);
            } break;

            default:
                panel(P(ctx));
                break;
        }

    }
    {_BOTTOM_(28);
        // Tabs
        for(int i = 0; i < BP_TAB_NONE_OR_NUM; i++) {
            auto tab = (Bottom_Panel_Tab)i;
            
            _LEFT_SLIDE_(120);

            String label = bottom_panel_tab_labels[tab];
            bool selected = (cui->open_bottom_panel_tab == tab);
            
            if(button(PC(ctx, i), label, true, selected) & CLICKED_ENABLED) {
                if(cui->open_bottom_panel_tab != tab)
                    cui->open_bottom_panel_tab = tab;
                else
                    cui->open_bottom_panel_tab = BP_TAB_NONE_OR_NUM;
            }
        }
    }
    

    // A little @Hacky. These are when we build the world_view UI element.
    // We need to do it here just because we want to have some elements over
    // the world view, and therefore need to build them before it.
    Rect world_view_a     = area(ctx.layout);
    m4x4 world_projection = world_projection_matrix(world_view_a);

    // Over world view
    { _AREA_COPY_();

        // CHAT // // @Temporary
        float chat_y = world_view_a.y + world_view_a.h;
        for(int i = room->num_chat_messages-1; i >= 0; i--) {
            auto *chat = &room->chat_messages[i];

            if(world_t - chat->t >= 30) continue;

            auto *e = find_player_entity(chat->user, room);
            if(!e) continue;
            Assert(e->type == ENTITY_PLAYER);

            v3 entity_p      = entity_position(e, world_t);
            v2 bottom_center = world_to_screen_space(entity_p + V3_Z * 5, world_view_a, world_projection);

            bottom_center.y = min(chat_y, bottom_center.y);
            if(bottom_center.y < chat_y) chat_y = bottom_center.y;

            // @Robustness: If we change how we draw chat messages we need to change this code that measures the string.
            const Font_Size fs = FS_14;
            const Font_ID font = FONT_BODY;
            
            float max_w = 160;
            Body_Text bt = create_body_text(chat->text, {0, 0, max_w, 0}, fs, font, client->fonts);

            float w = max_w;
            if(bt.lines.n == 1) w = body_text_line_width(0, &bt, client->fonts);

            Rect a;
            a.s = { w + 8, body_text_height(&bt) + 4 };
            a.x = bottom_center.x - a.w / 2.0f;
            a.y = bottom_center.y - a.h;
            
            chat_y -= a.h + 8;

            _AREA_(a);
            ui_chat(PC(ctx, i), chat->text);
        }
        
        // ACTION QUEUE //
        { _RIGHT_CUT_(48);
            if(player_entity != NULL) {
                Assert(player_entity->type == ENTITY_PLAYER);

                auto *player_e = &player_entity->player_e;
                for(int i = 0; i < player_e->action_queue_length; i++)
                {
                    auto *action = &player_e->action_queue[i];

                    String label = STRING("?");
                    
                    switch(action->type) {
                        case PLAYER_ACT_ENTITY: {
                            auto *entity_action = &action->entity;
                            
                            Assert(entity_action->target != NO_ENTITY);
                            Entity *target = find_entity(entity_action->target, room);

                            Assert(target); // @Norelease: This fails if this is a pick-up action and the action is already performed. But we shouldn't reach this point if the action already is performed.                    
                            Assert(target->type == ENTITY_ITEM); // @Temporary @Norelease: We will have player->player actions, right?

                            label = item_types[target->item_e.item.type].name;
                            Assert(label.length > 0);
                            label.length = 1;
                        } break;

                        case PLAYER_ACT_WALK: {
                            label = STRING("WALK");
                        } break;

                        default: Assert(false); break;
                    }
                    
                    _TOP_SQUARE_CUT_();
                    _SHRINK_(8);
                    bool enabled = false;
                    button(PC(ctx, i), label, enabled);
                }
            }
        }
    }
    
    auto *wv = world_view(P(ctx));
    

    bool ok_to_select_entity = true;
        
    // CLICK TILE //
    if(wv->clicked_tile_ix >= 0)
    {
        auto *user = &client->user;
        auto *room = &client->game.room;
        double world_t = world_time_for_room(room, t);
        Item *selected_item = get_selected_inventory_item(&client->user);
        
        if(wv->hovered_entity == NO_ENTITY || selected_item != NULL)
        {   
            C_RS_Action action = {0};
            action.type = C_RS_ACT_CLICK_TILE;
            auto &ct = action.click_tile;
            ct.tile_ix = wv->clicked_tile_ix;

            v3 tp = tp_from_index(ct.tile_ix);

            if(!selected_item || can_place_item_entity_at_tp(selected_item, tp, world_t, room))
            {
                if(selected_item) {
                    ct.item_to_place = *selected_item;
                } else {
                    Item no_item = {0};
                    no_item.id = NO_ITEM;
                    ct.item_to_place = no_item;
                }

                // @Norelease: IMPORTANT: Any time we want to enqueue a C_RS_Action, we should check that we're connected to a room!
                array_add(client->server_connections.room_action_queue, action);

                if(selected_item) {
                    Entity preview_entity = create_preview_item_entity(selected_item, tp_from_index(wv->clicked_tile_ix), world_t);

                    // NOTE: We add a preview entity and remove the placed item locally, before
                    //       we know if the operation succeeds.
                    //       We assumes the servers will send us a ROOM_UPDATE / USER_UPDATE on
                    //       transaction abort so we get our stuff back / know when to remove the
                    //       preview entity.

                    auto type = selected_item->type;
            
                    add_entity(preview_entity, room);
                    inventory_remove_item_locally(selected_item->id, user);

                    if(!in_array(input->keys, VKEY_SHIFT) ||
                       !select_next_inventory_item_of_type(type, user))
                    {
                        inventory_deselect(user);
                    }
                }
            
                ok_to_select_entity = false;
            }
        }
    }

    if(ok_to_select_entity &&
       wv->clicked_entity != NO_ENTITY)
    {
        // SELECT ENTITY //
        client->game.room.selected_entity = wv->clicked_entity;
    }
}


void client_mouse_down(Window *window, Mouse_Button button, void *client_)
{
    auto *client = (Client *)client_;
    auto &mouse = client->input.mouse;

    mouse.buttons      |= button;
    mouse.buttons_down |= button;
}

void client_mouse_up(Window *window, Mouse_Button button, void *client_)
{
    auto *client = (Client *)client_;
    auto &mouse = client->input.mouse;

    mouse.buttons    &= ~button;
    mouse.buttons_up |=  button;
}

void client_mouse_move(Window *window, int x, int y, u64 ms, void *client_)
{
    auto *client = (Client *)client_;
    auto &input = client->input;
    
    input.mouse.p = { (float)x, (float)y };
}

void client_character_input(Window *window, byte *utf8, int num_bytes, u16 repeat_count, void *client_)
{
    auto *client = (Client *)client_;
    auto *input  = &client->input;

    u8 *at  = utf8;
    u8 *end = utf8 + num_bytes;
    while(at < end) {
        u8 *cp_start = at;
        int cp = eat_codepoint(&at);
        
        if(cp == '\b') {
            if(input->text.n > 0) {
                u8 *at2 = input->text.e + input->text.n;
                int last_cp = eat_codepoint_backwards(&at2);
                if(last_cp != '\b') {
                    input->text.n = at2 - input->text.e;
                    continue;
                }
            }

            // If input->text is empty or only contains \b's, we add a \b. See comment in Input_Manager for an explanation of why we know this is true here.
            array_add(input->text, (u8)'\b');
            continue;
        }

        array_add(input->text, cp_start, at-cp_start);
    }
    
}

void client_key_down(Window *window, virtual_key key, u8 scan_code, u16 repeat_count, void *client_)
{
    Client *client = (Client *)client_;
    Input_Manager *input = &client->input;
    
    if(key == VKEY_RETURN)
        client_character_input(window, (byte *)"\n", 1, repeat_count, client_);

    ensure_in_array(input->keys, key);
    
    ensure_in_array(input->keys_down,   key);
    ensure_not_in_array(input->keys_up, key);

    for(u16 i = 0; i < repeat_count; i++)
        array_add(input->key_hits, key);
}

void client_key_up(Window *window, virtual_key key, u8 scan_code, void *client_)
{
    Client *client = (Client *)client_;
    Input_Manager *input = &client->input;

    ensure_not_in_array(input->keys, key);
    
    ensure_not_in_array(input->keys_down, key);
    ensure_in_array(input->keys_up, key);
}


void client_set_window_delegate(Window *window, Client *client)
{
    Window_Delegate delegate = {0};
    delegate.data = client;
    
    delegate.key_down = &client_key_down;
    delegate.key_up   = &client_key_up;
    
    delegate.character_input = &client_character_input;

    delegate.mouse_down = &client_mouse_down;
    delegate.mouse_up   = &client_mouse_up;
    delegate.mouse_move = &client_mouse_move;

    window->delegate = delegate;
}

// NOTE: *_game should already have been zeroed.
void init_game(Game *_game)
{

}

void update_client(Client *client, Input_Manager *input)
{
    // ESCAPE KEY //
    if(in_array(input->keys_down, VKEY_ESCAPE)) {
        
        if(client->user.selected_inventory_item_plus_one > 0) {
            inventory_deselect(&client->user);
        }
    }
}

int client_entry_point(int num_args, char **arguments)
{

#if 0
    //no_checkin
    String obj_contents = {0};
    if(!read_entire_resource("meshes/bed.obj", &obj_contents.data, ALLOC_TMP, &obj_contents.length)){
        Debug_Print("Unable to read mesh resource.\n");
        return 987;
    }
    if(read_obj(obj_contents)) {
        Debug_Print("Read obj successfully.\n");
    } else {
        Debug_Print("Failed to read obj.\n");
    }
    return 0;
    //---
#endif
                             
    platform_init_socket_use();
    
    String_Builder sb = {0};
    Debug_Print("I am a client.\n");
    
    // INIT CLIENT //
    Client client = {0};
    create_mutex(client.mutex);
    
    Layout_Manager *layout = &client.layout;
    UI_Manager     *ui = &client.ui;
    Input_Manager  *input = &client.input;
    Window *main_window = &client.main_window;
    //--

    // @Norelease: Doing Developer stuff in release build...
    init_developer(&client.developer);
    load_tweaks(client.developer.user_id, &sb);
#if OS_WINDOWS
    setup_tweak_hotloading();
#endif

    // CREATE WINDOW //
    v4s window_rect = tweak_v4s(TWEAK_INITIAL_OS_WINDOW_RECT);
    platform_create_window(main_window, "Citrus", window_rect.w, window_rect.h, window_rect.x, window_rect.y);
    client_set_window_delegate(main_window, &client);
    platform_get_window_rect(main_window, &client.main_window_a.x,  &client.main_window_a.y,  &client.main_window_a.w,  &client.main_window_a.h);
    //--

    // INIT GAME //
    init_game(&client.game);
    //--

    // INIT CLIENT UI //
    init_client_ui(&client.cui);
    //--

    // START RENDER LOOP //
    Render_Loop render_loop = {0};
    if(!start_render_loop(&render_loop, &client))
    {
        Debug_Print("Unable to start render loop.\n");
        return 1;
    }
    //--

    // START NETWORK LOOP //
    Network_Loop network_loop = {0};
    if(!start_network_loop(&network_loop, &client))
    {
        Debug_Print("Unable to start network loop.\n");
        return 1;
    }
    //--

                         
    // UI SETUP //
    UI_Context ui_ctx = UI_Context();
    ui_ctx.manager = ui;
    ui_ctx.layout  = layout;
    ui_ctx.client  = &client;
    //--

#if DEBUG || true // Debug stuff for keeping track of things like FPS and UPS.
    u64 last_second = 0;
#endif
   
    while(true)
    {
        new_input_frame(input);

        Assert((u64)wglGetCurrentContext() == 0);
        
        // TIME //
        double t = platform_get_time();
        u64 second = t;
        // //// //
        
        // GET WINDOW SIZE, INIT LAYOUT MANAGER //
        Rect window_a;        
        platform_get_window_rect(main_window, &window_a.x, &window_a.y, &window_a.w, &window_a.h);
        layout->root_size = window_a.s;
        // //////////////////////////////////// //

        // NOTE: Get input as close as possible to the UI update.
        if(!platform_process_input(main_window, true)) {
            break;
        }

        Cursor_Icon cursor;
        
        lock_mutex(client.mutex);
        {

            // MAYBE RELOAD TWEAKS //
#if OS_WINDOWS
            {
                bool old_color_tiles_by_position = tweak_bool(TWEAK_COLOR_TILES_BY_POSITION);
                {
                    maybe_reload_tweaks(client.developer.user_id, &sb);
                }
                bool new_color_tiles_by_position = tweak_bool(TWEAK_COLOR_TILES_BY_POSITION);

#if DEBUG
                if(new_color_tiles_by_position != old_color_tiles_by_position) {
                    client.game.room.static_geometry_up_to_date = false;
                }
#endif
            }
#endif
            // ------------------ //
            
            client.main_window_a = window_a;

            update_client(&client, input);
            
            // BUILD UI //
            begin_ui_build(ui);
            {
                push_area_layout(window_a, layout);
            
                client_ui(P(ui_ctx), input, t, &client);
            
#if DEBUG || true
                if(second != last_second) {
                    ups = updates_this_second;
                    updates_this_second = 0;
                }
                updates_this_second++;

                char *title = concat_cstring_tmp("Citrus | ", fps, " FPS | ", ups, " UPS | ", draw_calls_last_frame, " draws | ", sb);
                title = concat_cstring_tmp(title, triangles_this_frame, " tris", sb);
                
                platform_set_window_title(main_window, title);
#endif
                pop_layout(layout);

            }
            double t = platform_get_time(); // @Robustness: This is safe to do, right?
            end_ui_build(ui, &client.input, client.fonts, t, &client.game.room, &cursor);
            // //////// //

            reset_temporary_memory();
        }
        unlock_mutex(client.mutex);

#if DEBUG || true
        last_second = second;
#endif
        
        platform_set_cursor_icon(cursor);
    }

    stop_network_loop(&network_loop, &client);
    stop_render_loop(&render_loop, &client);

    platform_deinit_socket_use();
 
    return 0;
}
