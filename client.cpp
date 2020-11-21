


// @Temporary
void bar_window(UI_Context ctx, Input_Manager *input)
{
    U(ctx);

    _CENTER_X_(320);

    int c = 2;

    const Allocator_ID allocator = ALLOC_APP;

    static String the_string = copy_cstring_to_string("But I must explain to you how all this mistaken idea of denouncing pleasure and praising pain was born and I will give you a complete account of the system, and expound the actual teachings of the great explorer of the truth, the master-builder of human happiness. No one rejects, dislikes, or avoids pleasure itself, because it is pleasure, but because those who do not know how to pursue pleasure rationally encounter consequences that are extremely painful. Nor again is there anyone who loves or pursues or desires to obtain pain of itself, because it is pain, but because occasionally circumstances occur in which toil and pain can procure him some great pleasure.", allocator);


    UI_ID window_id;
    { _AREA_(begin_window(P(ctx), &window_id, STRING("REFRIGERATOR")));

        _GRID_(1, c, 4);

        for(int i = 0; i < 1 * c; i++) {
            _CELL_();

            if(i == 0) {
                //ui_text(the_string, PC(ctx, i));
                bool text_did_change;
                String text = textfield_tmp(the_string, input, PC(ctx, i), &text_did_change);
                if(text_did_change) {
                    // @Speed!!!
                    clear(&the_string, allocator);
                    the_string = copy_of(&text, allocator);
                }
            } else {
                bool text_did_change;
                String text = textfield_tmp(the_string, input, PC(ctx, i), &text_did_change);
                if(text_did_change) {
                    // @Speed!!!
                    clear(&the_string, allocator);
                    the_string = copy_of(&text, allocator);
                }
            }
        }
    }
    end_window(window_id, ctx.manager);
}

void request_connection_to_room(Room_ID id, Client *client)
{
    Assert(client->server_connections.room_connect_requested == false);
    
    client->server_connections.requested_room = id;
    client->server_connections.room_connect_requested = true;
}

// @Temporary
bool foo_window(UI_Context ctx, Client *client)
{
    U(ctx);
    
    bool result = false;

    float room_button_h = 64;

    // DETERMINE WINDOW RECT //
    Rect window_a;
    {
        Rect a = area(ctx.layout);
        float map_s = min(a.w, a.h);
        Rect map_a = { a.x, a.y, map_s, map_s };
        float border_and_padding = window_border_width + window_default_padding;
        v4 borders_and_stuff = { border_and_padding,
                                 border_and_padding,
                                 border_and_padding + window_title_height + room_button_h + window_default_padding,
                                 border_and_padding };
        map_a = shrunken(map_a, borders_and_stuff);
        map_s = min(map_a.w, map_a.h);
        map_a.w = map_s;
        map_a.h = map_s;
        window_a = grown(map_a, borders_and_stuff);
    }
    _AREA_(window_a);
    // 

    // @Temporary
    String_Builder sb = {0};
    
    const int num_rooms = 15;

    Room_ID requested_room = (client->server_connections.room_connect_requested) ? client->server_connections.requested_room : -1;
    Room_ID current_room   = client->server_connections.room.current_room;

    UI_ID window_id;
    { _AREA_(begin_window(P(ctx), &window_id, STRING("FOO")));
        { _TOP_CUT_(room_button_h);
            _GRID_(num_rooms, 1, 4);
            for(int r = 0; r < num_rooms; r++)
            {
                _CELL_();
                bool selected;
                if(requested_room != -1) selected = (requested_room == r);
                else                     selected = (current_room   == r);

                if(button(PC(ctx, r), concat_tmp("", r, sb), (requested_room != -1), selected) & CLICKED_ENABLED)
                {
                    request_connection_to_room((Room_ID)r, client);
                }
            }
        }

        cut_top(window_default_padding, ctx.layout);

        u64 clicked_tile = world_view(P(ctx));
        if(clicked_tile != U64_MAX)
        {
            // @Cleanup: We don't know what kind of system is best to keep track of "requests" to the server yet.
            //           We probably will want to know which operations succeeds and fails.
            //           And for some things we want to get stuff back.
            RSB_Packet(client, Click_Tile, clicked_tile);
        }
        
    }
    UI_Click_State close_button_state;
    end_window(window_id, ctx.manager, &close_button_state);
    if(close_button_state & CLICKED_ENABLED)
        result = true;

    return result;
}

void client_ui(UI_Context ctx, Input_Manager *input, Client *client)
{
    U(ctx);

    Rect a = area(ctx.layout);

    _SHRINK_(10);    
#if 1

    { _RIGHT_HALF_();
        foo_window(P(ctx), client);
    }
    { _LEFT_HALF_();
        _BOTTOM_HALF_();
        bar_window(P(ctx), input);
    }
#else
    foo_window(false, P(ctx), client);
#endif
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

int client_entry_point(int num_args, char **arguments)
{
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
        u64 millisecond = platform_milliseconds();
        u64 second = millisecond / 1000;
        double t = millisecond / 1000.0;
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
#if OS_WINDOWS
            maybe_reload_tweaks(client.developer.user_id, &sb);
#endif
            
            client.main_window_a = window_a;
            
            
            // BUILD UI //
            begin_ui_build(ui);
            {
                push_area_layout(window_a, layout);
            
                client_ui(P(ui_ctx), input, &client);
            
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
            double t = platform_milliseconds() / 1000.0; // @Robustness: This is safe to do, right?
            end_ui_build(ui, &client.input, client.fonts, t, &cursor);
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
