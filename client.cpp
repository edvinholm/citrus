

void request_connection_to_room(Room_ID id, Client *client)
{
    Assert(client->connections.room_connect_requested == false);
    
    client->connections.requested_room = id;
    client->connections.room_connect_requested = true;
}

void request_connection_to_user(User_ID user_id, Client *client)
{
    Assert(client->connections.user_connect_requested == false);

    client->connections.requested_user = user_id;
    client->connections.user_connect_requested = true;
}

void request_connection_to_market(Client *client)
{
    Assert(client->connections.market_connect_requested == false);

    client->connections.market_connect_requested = true;
}


User_ID current_user_id(Client *client)
{
    if(!client->connections.user.connected) return NO_USER;
    if(!client->user.initialized) return NO_USER;

    Assert(client->user.id == client->connections.user.current_user);
    return client->user.id;
}

User *current_user(Client *client)
{
    if(current_user_id(client) == NO_USER) return NULL;
    return &client->user;
}

Entity *find_current_player_entity(Client *client)
{
    User_ID user_id = current_user_id(client);
    if(user_id == NO_USER) return NULL;

    return find_player_entity(user_id, &client->room);
}

bool action_of_type_in_queue(C_MS_Action_Type type, Client *client)
{
    auto &queue = client->connections.market_action_queue;
    for(int i = 0; i < queue.n; i++) {
        if(queue[i].type == type) return true;
    }

    return false;
}

bool action_of_type_in_queue(C_RS_Action_Type type, Client *client)
{
    auto &queue = client->connections.room_action_queue;
    for(int i = 0; i < queue.n; i++) {
        if(queue[i].type == type) return true;
    }

    return false;
}


void request(Player_Action *action, Client *client)
{
    C_RS_Action c_rs_act = {0};
    c_rs_act.type = C_RS_ACT_PLAYER_ACTION;
    c_rs_act.player_action = *action;

    array_add(client->connections.room_action_queue, c_rs_act);
}

bool predicted_possible(Player_Action *action, double world_t, Client *client, Player_Action_Prediction_Info *_prediction_info = NULL)
{
    Entity *player = find_current_player_entity(client);
    if(player == NULL) return false;

    User *user = current_user(client);
    if(!user) return false;

    Assert(player->type == ENTITY_PLAYER);

    auto *room = &client->room;

    Player_State player_state = player_state_after_completed_action_queue(player, world_t, room);
    while(true) {
        Optional<Player_Action> action_needed_before = {0};
        if(player_action_predicted_possible(action, &player_state,
                                            world_t, room, &action_needed_before,
                                            NULL, NULL, _prediction_info, user)) {
            return true;
        }
        else {
            Player_Action act;
            if(get(action_needed_before, &act)) {
        
                if(player->player_e.action_queue_length >= ARRLEN(player->player_e.action_queue)-1) return false;

                // NOTE: @Cleanup Unclear: apply_actions_to_player_state returns true if the action
                //                         is predicted possible. This is so we don't need to do
                //                         predicted_possible() first, and then apply_actions().
                //                         This is for @Speed.
                if(!apply_actions_to_player_state(&player_state, &act, 1, world_t, room, user))
                    return false;
                
                continue;
            }
        }
        break;
    }
    
    return false;
}

bool request_if_predicted_possible(Player_Action *action, double world_t, Client *client)
{
    if(!predicted_possible(action, world_t, client)) return false;
    request(action, client);
    return true;
}

void request(Entity_Action *action, Entity_ID target, Client *client)
{
    Player_Action player_action = make_player_entity_action(action, target);
    request(&player_action, client);
}

bool predicted_possible(Entity_Action *action, Entity_ID target, double world_t, Client *client)
{
    Player_Action player_action = make_player_entity_action(action, target);
    return predicted_possible(&player_action, world_t, client);
}

bool request_if_predicted_possible(Entity_Action *action, Entity_ID target, double world_t, Client *client)
{
    Player_Action player_action = make_player_entity_action(action, target);
    return request_if_predicted_possible(&player_action, world_t, client);
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
    
    bool connected  = client->connections.user.connected;
    bool connecting = client->connections.user_connect_requested;

    _WINDOW_(P(ctx), user->username, true, opt(user->color, connected));

    if(connected) {
        _TOP_CUT_(480 + 72);
        cut_bottom(4, ctx.layout);

        { _BOTTOM_CUT_(72);
            Money available = user->money - user->reserved_money;
            ui_text(P(ctx), concat_tmp("MONEY [ Available: ¤", available, ", Total: ¤", user->money, " ]"));
        }

        int cols = 8;
        int rows = 14;
        _GRID_(cols, rows, 2);
        for(int r = 0; r < rows; r++) {
            for(int c = 0; c < cols; c++) {
                _CELL_();

                auto cell_ix = r * cols + c;
                Inventory_Slot *slot = &user->inventory[cell_ix];
                Item *item = NULL;
                if(slot->flags & INV_SLOT_FILLED) item = &slot->item;
                
                bool enabled = (item != NULL) && !(slot->flags & INV_SLOT_RESERVED);

                bool selected = (cell_ix == user->selected_inventory_item_plus_one - 1);
                if(ui_inventory_slot(PC(ctx, cell_ix), slot, enabled, selected) & CLICKED_ENABLED) {
                    // @Norelease @Robustness: Make this an item ID.
                    //   So if the gets removed or moved on the server, the item will be
                    //   deselected, and not replaced by another item that takes its slot
                    //   in the inventory.
                    user->selected_inventory_item_plus_one = cell_ix + 1;
                }
            }
        }
    }
    
    
    auto *us_con = &client->connections.user;
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

void request_market_set_view(Market_View_Target target, Market *market, Client *client)
{
    C_MS_Action action = {0};
    action.type = C_MS_SET_VIEW;

    auto *x = &action.set_view;
    x->target = target;
    
    array_add(client->connections.market_action_queue, action);// @Norelease: IMPORTANT: Any time we want to enqueue a C_MS_Action, we should check that we're connected to a market server!

    market->waiting_for_view_update = true;
}


void market_article_view(UI_Context ctx, Market_View *view, bool controls_enabled, Input_Manager *input, Market_UI *mui, Market *market, Client *client, User *user = NULL, Item *inventory_item = NULL)
{
    U(ctx);
    
    Assert(view->target.type == MARKET_VIEW_TARGET_ARTICLE);
    auto *article_view   = &view->article;
    auto *article_target = &view->target.article;
    auto article_id   = article_target->article; // @Jai: using article_target.
    auto price_period = article_target->price_period;
    
    bool connecting = client->connections.market_connect_requested;
    bool connected  = !connecting && client->connections.market.connected;

    
    // PLACE ORDER //
    { _BOTTOM_CUT_(40);

        { _SHRINK_(4); // Panel padding

            // NOTE: We don't check that a C_MS_PLACE_ORDER is not queued here. Because
            //       all data net_client needs is included in the action struct as of
            //       2021-02-08. I don't know about user experience though, maybe you
            //       don't want to be able to place a new order until you see that the
            //       first one successfully was uploaded to the market server.... But
            //       that's an issue for later. (@Norelease).
            bool order_controls_enabled = controls_enabled; 

            // PRICE, SELL, BUY //
            { _RIGHT_CUT_(200);
                _GRID_(2, 1, window_default_padding);

                C_MS_Action action = {0};
                action.type = C_MS_PLACE_ORDER;

                auto *order  = &action.place_order;
                order->price = mui->order_draft.price;

                bool do_enqueue_action = false;

                bool place_order_buttons_enabled = (order_controls_enabled &&
                                                    article_id != ITEM_NONE_OR_NUM &&
                                                    !action_of_type_in_queue(C_MS_PLACE_ORDER, client));

                // BUY BUTTON //
                { _CELL_();

                    bool enabled = false;

                    if(user)
                    {
                        Money available_money = user->money - user->reserved_money;
                
                        enabled = (place_order_buttons_enabled     &&
                                   inventory_item == NULL          &&
                                   available_money >= order->price &&
                                   inventory_has_available_space_for_item_type(article_id, user));
                    }
                
                    if(button_colored(P(ctx), C_BUY, STRING("BUY"), enabled) & CLICKED_ENABLED)
                    {
                        order->is_buy_order = true;
                        order->buy.item_type = article_id;
                        do_enqueue_action = true;
                    }
                }

                // SELL BUTTON //
                { _CELL_();
                    bool enabled = place_order_buttons_enabled && (inventory_item != NULL);
                    if(button_colored(P(ctx), C_SELL, STRING("SELL"), enabled) & CLICKED_ENABLED) {
                        order->is_buy_order = false;
                        order->sell.item_id = inventory_item->id;
                        do_enqueue_action = true;

                        Assert(user);
                        if(in_array(input->keys, VKEY_SHIFT)) // @Norelease @Robustness :InputFrames: We need to know if shift was down when the button was clicked. The click can have happened a few input frames back. -EH, 2021-02-12
                            select_next_inventory_item_of_type(inventory_item->type, user);
                    }
                }

                if(do_enqueue_action) {
                    Assert(!action_of_type_in_queue(C_MS_PLACE_ORDER, client));
                    array_add(client->connections.market_action_queue, action); // @Norelease: IMPORTANT: Any time we want to enqueue a C_MS_Action, we should check that we're connected to a market server!
                }
            }
            cut_right(window_default_padding, ctx.layout);

        
            // PRICE TEXTFIELD //
            { _RIGHT_CUT_(100);            
                auto *price = &mui->order_draft.price;
                *price = textfield_s64(P(ctx), *price, input, order_controls_enabled);
            }

        }
        panel(P(ctx));
    }


    // ARTICLE DETAILS //
    if(article_id != ITEM_NONE_OR_NUM)
    {
        Assert(article_view);
        Assert(article_id >= 0 &&
               article_id < ARRLEN(item_types));
               
        Item_Type *item_type = &item_types[article_id];

        // HEADER
        { _TOP_CUT_(48);

            // Item Preview
            { _LEFT_SQUARE_CUT_();
                button(P(ctx)); // @Norelease: Item preview here.
            }
            cut_left(window_default_padding * 1.5f, ctx.layout);

            // Current price
            { _RIGHT_CUT_(96);
                cut_bottom(2, ctx.layout); // To get it to line up with the name

                Money price = (article_view->num_prices == 0) ? 0 : article_view->prices[article_view->num_prices-1];
                String price_str = concat_tmp("¤", price);
                ui_text(P(ctx), price_str, FS_28, FONT_BODY, HA_RIGHT, VA_BOTTOM);
            }
            cut_right(window_default_padding * 1.5f, ctx.layout);

            // Name
            ui_text(P(ctx), item_type->name, FS_36, FONT_TITLE, HA_LEFT, VA_BOTTOM);
        }
        cut_top(window_default_padding, ctx.layout);

        // GRAPH
        { _TOP_CUT_(min(300, area(ctx.layout).h - window_default_padding));

            // Period selection
            { _BOTTOM_CUT_(28);

                bool small_buttons = (area(ctx.layout).w < 395);

                { _SHRINK_(3);
                    for(int i = PERIOD_NONE_OR_NUM-1; i >= 0; i--)
                    {    
                        Price_Period period = (Price_Period)i;
                        
                        if(i < PERIOD_NONE_OR_NUM-1) slide_right(2, ctx.layout);

                        String label = price_period_names[i];
                        if(small_buttons) label.length = min(1, label.length);
                        
                        _RIGHT_SLIDE_((small_buttons) ? 25 : 64);
                        bool selected = (price_period == period);
                        if(button(PC(ctx, i), label, !selected, selected) & CLICKED_ENABLED)
                        {
                            Market_View_Target view_target;
                            Zero(view_target);
            
                            view_target.type = MARKET_VIEW_TARGET_ARTICLE;
                            view_target.article.article      = article_id;
                            view_target.article.price_period = period;
         
                            request_market_set_view(view_target, market, client);
                        }
                    }

                    ui_text(P(ctx), STRING("PERIOD:"), FS_16, FONT_BODY, HA_RIGHT, VA_CENTER);
                }
                panel(P(ctx));
            }
            cut_bottom(2, ctx.layout);

            
            // The graph
            
            // @Cleanup
            float values[ARRLEN(article_view->prices)];
            for(int i = 0; i < article_view->num_prices; i++) {
                values[i] = (float)article_view->prices[i];
            }
            int num_values = article_view->num_prices;
            
            // @Cleanup
            float min = FLT_MAX;
            float max = 1;
            for(int i = 0; i < article_view->num_prices; i++)
            {
                float price = (float)article_view->prices[i];
                Assert(price >= 0);

                if(price < min) min = price;
                if(price > max) max = price;
            }

            if(min > max) {
                Assert(article_view->num_prices == 0);
                min = 0;
            }
            else if (floats_equal(min, max)) {
                // Put the line in the middle.
                min  = max * 0.5f;
                max += min;
            }

            // Some margin
            min -= (max - min) * 0.05f;
            max += (max - min) * 0.05f;

            graph(P(ctx), values, num_values, min, max);
        }

        
        
    }
    // --

}

void market_orders_view(UI_Context ctx, Market_View *view, bool controls_enabled)
{
    U(ctx);

    Assert(view->target.type == MARKET_VIEW_TARGET_ORDERS);
    auto *orders_view = &view->orders;

    ui_text(P(ctx), STRING("Hello, Sailor!"), FS_48, FONT_BODY, HA_CENTER, VA_CENTER);
}


void market_window(UI_Context ctx, double t, Input_Manager *input, Market_UI *mui, Client *client)
{
    U(ctx);

    const Price_Period default_price_period = PERIOD_HOUR;
    
    User *user = current_user(client);
    auto *market = &client->market;
    auto *view   = &market->view;

    auto *article_view_target = &view->target.article;
    auto *article_view        = &view->article;
    if(view->target.type != MARKET_VIEW_TARGET_ARTICLE) {
        article_view_target = NULL;
        article_view        = NULL;
    }
    

    if(market->ui_needs_update) {
        if(article_view)
        {
            auto price = (article_view->num_prices == 0) ? 0 : article_view->prices[article_view->num_prices-1];
            mui->order_draft.price = price;
        }
        
        market->ui_needs_update = false;
    }
    
    
    bool connecting = client->connections.market_connect_requested;
    bool connected  = !connecting && client->connections.market.connected;

    String window_title;
    if(connected)       window_title = STRING("MARKET [CONNECTED]");
    else if(connecting) window_title = STRING("MARKET [CONNECTING]");
    else                window_title = STRING("MARKET [DISCONNECTED]");

    
    Item *inventory_item = NULL;
    if(user) inventory_item = get_selected_inventory_item(user);

    // Set view to article of selected inventory item type.
    if(inventory_item != NULL) {
        if((!article_view_target || article_view_target->article != inventory_item->type) && 
            !market->waiting_for_view_update)
        {
            Market_View_Target view_target;
            Zero(view_target);
            
            view_target.type = MARKET_VIEW_TARGET_ARTICLE;
            view_target.article.article      = inventory_item->type;
            view_target.article.price_period = (article_view_target) ? article_view_target->price_period : default_price_period;
            
            request_market_set_view(view_target, market, client);
        }
    }

    
    bool controls_enabled = connected && !market->waiting_for_view_update;
    


    _THEME_(C_THEME_MARKET);
    _WINDOW_(P(ctx), window_title, true, {0}, {0}, opt<v2>({ 340, 320 }));



    // ARTICLE LIST / SEARCH / ORDERS BUTTON //
    if(area(ctx.layout).w >= 554)
    {
        _LEFT_CUT_(150);

        bool sidebar_controls_enabled = controls_enabled;

        // SEARCH FIELD
        { _TOP_CUT_(32);
            bool text_did_change;
            textfield_tmp(P(ctx), STRING("Foobar"), input, &text_did_change, sidebar_controls_enabled); // @Norelease
        }
        cut_top(4, ctx.layout);

        // ORDERS BUTTON
        { _BOTTOM_CUT_(40);

            { _SHRINK_(4);
                
                bool selected = view->target.type == MARKET_VIEW_TARGET_ORDERS;
                bool enabled  = !selected && sidebar_controls_enabled;
            
                if(button(P(ctx), STRING("PLACED ORDERS"), enabled, selected, opt<v4>({ 0.02, 0.28, 0.23, 1 })) & CLICKED_ENABLED)
                {
                    Market_View_Target target;
                    Zero(target);
                    target.type = MARKET_VIEW_TARGET_ORDERS;

                    request_market_set_view(target, market, client);
                }
            }

            panel(P(ctx));
        }
        cut_bottom(4, ctx.layout);

        // @Norelease: Scroll area for the article list...
        
        // ITEM TYPES //
        for(int i = 0; i < ITEM_NONE_OR_NUM; i++)
        {
            cut_top(2, ctx.layout);
            
            _TOP_SLIDE_(36);
            
            Item_Type_ID type_id = (Item_Type_ID)i;
            auto *type = &item_types[type_id];
            bool selected = (article_view_target && article_view_target->article == type_id);
            
            if(button(PC(ctx, i), type->name, sidebar_controls_enabled, selected) & CLICKED_ENABLED)
            {       
                Market_View_Target view_target;
                Zero(view_target);
            
                view_target.type = MARKET_VIEW_TARGET_ARTICLE;
                view_target.article.article      = type_id;
                view_target.article.price_period = (article_view_target) ? article_view_target->price_period : default_price_period;
         
                request_market_set_view(view_target, market, client);
                
                if(user) inventory_deselect(user);
            }
            
        }
    }
    cut_left(window_default_padding, ctx.layout);


    // CURRENT VIEW //
    switch(view->target.type) {
        case MARKET_VIEW_TARGET_ORDERS: {
            market_orders_view(P(ctx), view, controls_enabled);
        } break;

        case MARKET_VIEW_TARGET_ARTICLE: {
            market_article_view(P(ctx), view, controls_enabled, input, mui, market, client, user, inventory_item);
        } break;
    }
    
}


String entity_action_label(Entity_Action action)
{
    switch(action.type) {
        case ENTITY_ACT_PICK_UP: return STRING("PICK UP"); break;
        case ENTITY_ACT_PLACE_IN_INVENTORY: return STRING("PLACE IN INVENTORY"); break;
        case ENTITY_ACT_HARVEST: return STRING("HARVEST"); break;
        case ENTITY_ACT_SET_POWER_MODE: {
            auto *x = &action.set_power_mode;
            if(x->set_to_on) {
                return STRING("START");
            } else {
                return STRING("STOP");
            }
        } break;
        case ENTITY_ACT_WATER:      return STRING("WATER"); break;
        case ENTITY_ACT_CHESS: {
            auto *chess_action = &action.chess;
            switch(chess_action->type) {
                case CHESS_ACT_MOVE: return STRING("CHESS MOVE"); break;
                default:             return STRING("CHESS ???"); break;
            }
        }
        case ENTITY_ACT_SIT_OR_UNSIT: {
            auto *x = &action.sit_or_unsit;
            if(x->unsit) return STRING("STAND UP");
            else         return STRING("SIT");
        } break;

        default: Assert(false); return STRING("???"); break;
    }

    Assert(false);
    return EMPTY_STRING;
}

void chess_player(UI_Context ctx, Chess_Player *cp)
{
    U(ctx);
    
    String str = concat_tmp(cp->user, " | KING: ", (cp->flags & KING_MOVED), ", BL: ", (cp->flags & BOTTOM_LEFT_MODIFIED), ", BR:", (cp->flags & BOTTOM_RIGHT_MODIFIED));
    ui_text(P(ctx), str);
}

void item_chess_tab(UI_Context ctx, Item *item, bool controls_enabled, Entity *e, Entity *player, double world_t, Client *client)
{
    U(ctx);

    Assert(item->type == ITEM_CHESS_BOARD);
    Assert(e);
    Assert(player);

    User_ID user_id = current_user_id(client);
            
    auto *board = &e->item_e.chess_board;
    auto *local = &e->item_local.chess;

    { _TOP_CUT_(42);
        _GRID_(2, 1, window_default_padding);

        for(int j = 0; j < 2; j++)
        {
            _CELL_();
            auto *cp = (j == 0) ? &board->white_player : &board->black_player;

            if(cp->user != NO_USER) chess_player(PC(ctx, j), cp);
            else {
                
                Entity_Action action = {0};
                action.type = ENTITY_ACT_CHESS;
                action.chess.type = CHESS_ACT_JOIN;
                action.chess.join.as_black = (cp == &board->black_player);

                bool selected = false;
                            
                // FIND JOIN ACTION // @Cleanup: Macro? Or wait for @Jai.
                for(int i = 0; i < player->player_e.action_queue_length; i++)
                {
                    auto *player_action = &player->player_e.action_queue[i];
        
                    if(player_action->type != PLAYER_ACT_ENTITY) continue;
                    auto *entity_action = &player_action->entity;
        
                    if(entity_action->target      != e->id) continue;
                    if(entity_action->action.type != ENTITY_ACT_CHESS) continue;
        
                    auto *chess_action = &entity_action->action.chess;
                    if(chess_action->type != CHESS_ACT_JOIN) continue;
                    if(chess_action->join.as_black != action.chess.join.as_black) continue;

                    selected = true;
                    break;
                }

                bool enabled  = controls_enabled && predicted_possible(&action, e->id, world_t, client);
                // @Norelease: Check if another player has this action queued.
                if(button(PC(ctx, j), STRING("JOIN"), enabled, selected) & CLICKED_ENABLED)
                {
                    request(&action, e->id, client);
                }
            }
        }
        
    }
    cut_top(window_default_padding, ctx.layout);
    
    _TOP_SQUARE_CUT_();

    
    // FIND QUEUED MOVE // @Cleanup: Macro? Or wait for @Jai.
    Chess_Move *queued_move = NULL;
    for(int i = 0; i < player->player_e.action_queue_length; i++)
    {
        auto *player_action = &player->player_e.action_queue[i];
        
        if(player_action->type != PLAYER_ACT_ENTITY) continue;
        auto *entity_action = &player_action->entity;
        
        if(entity_action->target      != e->id) continue;
        if(entity_action->action.type != ENTITY_ACT_CHESS) continue;
        
        auto *chess_action = &entity_action->action.chess;
        if(chess_action->type != CHESS_ACT_MOVE) continue;

        Assert(!queued_move); // We should have at most one queued chess move!
        queued_move = &chess_action->move;
    }


    
    UI_Chess_Board *ui_board = ui_chess_board(P(ctx), board, /*enabled = */controls_enabled,
                                              local->selected_square_ix_plus_one - 1, queued_move);

    if(ui_board->clicked_square_ix >= 0) {

        bool do_select = true;
        
        if(local->selected_square_ix_plus_one > 0)
        {
            Chess_Move move = {0};
            move.from = local->selected_square_ix_plus_one-1;
            move.to   = ui_board->clicked_square_ix;

            bool would_be_possible_if_our_turn;
            if(chess_move_possible_for_user(move, player->player_e.user_id, board, &would_be_possible_if_our_turn))
            {
                // ENQUEUE CHESS MOVE ACTION
                Entity_Action act = {0};
                act.type = ENTITY_ACT_CHESS;
                act.chess.type = CHESS_ACT_MOVE;
                act.chess.move = move;

                request_if_predicted_possible(&act, e->id, world_t, client);
                // /////////////////////// //

                local->selected_square_ix_plus_one = 0;    
                do_select = false;
            }
            else if(would_be_possible_if_our_turn) {
                do_select = false;
            }
        }

        if(do_select) {
            local->selected_square_ix_plus_one = ui_board->clicked_square_ix + 1;
        }
                        
    }
}


void item_info_tab(UI_Context ctx, Item *item, bool controls_enabled, Client *client, Entity *e = NULL, Entity *player = NULL, double world_t = 0)
{
    U(ctx);
        
    Assert(item != NULL);
    Assert(item->type != ITEM_NONE_OR_NUM);

    auto *type = &item_types[item->type];

    
    // ACTIONS //
    if(e && player) {

        Assert(player->type == ENTITY_PLAYER);  
        Assert(e->type == ENTITY_ITEM);

        auto *player_local = &player->player_local;

        Array<Entity_Action, ALLOC_TMP> actions = {0};
        auto state = player_state_after_completed_action_queue(player, world_t, &client->room);
        get_available_actions_for_entity(e, &state, &actions);

        for(int i = 0; i < actions.n; i++) {

            if(i > 0) { _BOTTOM_CUT_(window_default_padding); }
                
            _BOTTOM_CUT_(32);

            auto entity_action = actions[i];
                
            bool enabled = controls_enabled && predicted_possible(&entity_action, e->id, world_t, client);
                
            if(button(PC(ctx, i), entity_action_label(entity_action), enabled) & CLICKED_ENABLED)
            {
                request_if_predicted_possible(&entity_action, e->id, world_t, client);
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
            String volume_str = concat_tmp("Dimensions: ", vol.x, "x", vol.y, "x", vol.z);
            ui_text(P(ctx), volume_str);
        }
            
        { _TOP_CUT_(20);
            String owner_str = concat_tmp("Owner: ", item->owner);
            ui_text(P(ctx), owner_str);
        }

#if DEBUG
        if(e)
        { _TOP_CUT_(20);
            String id_str = concat_tmp("Entity: ", e->id);
            ui_text(P(ctx), id_str);
        }
        
        { _TOP_CUT_(20);
            String id_str = concat_tmp("Item: ", item->id.origin, ":", item->id.number);
            ui_text(P(ctx), id_str);
        }
#endif
    }


    if(item_types[item->type].container_type == LIQUID_CONTAINER) {
        { _TOP_CUT_(40);
            ui_liquid_container(P(ctx), &item->liquid_container, liquid_container_capacity(item));
        }

        // @Cleanup: move to ui_liquid_container.
        if(liquid_type_of_container(&item->liquid_container) == LQ_YEAST_WATER)
        {
            auto *yw = &item->liquid_container.liquid.yeast_water;
            
            { _TOP_CUT_(20);
                int pre  = yw->yeast / 10; // @Cleanup
                int post = yw->yeast % 10;
                String id_str = concat_tmp("Yeast: ", pre, ".", post, "%");
                ui_text(P(ctx), id_str);
            }
            { _TOP_CUT_(20);
                int pre  = yw->nutrition / 10; // @Cleanup
                int post = yw->nutrition % 10;
                String id_str = concat_tmp("Nutrition: ", pre, ".", post, "%");
                ui_text(P(ctx), id_str);
            }
        }
    }

    
    switch(item->type) {
        case ITEM_PLANT: {
            auto *plant = &item->plant;
                    
            { _TOP_CUT_(20);   
                float grow_progress = plant->grow_progress;
                String grow_str = concat_tmp("Grow progress: ", (int)(grow_progress * 100.0f), "%");
                ui_text(P(ctx), grow_str);
            }
        } break;
    }    
}

// NOTE: If the item is on an entity, REMEMBER to do update_entity_item()!
// REMEMBER: world_t is local to the Room Server we're on. User Server has no world_t.
//           So passing a world_t but no entity does not make sense... -EH, 2021-01-30
void item_window(UI_Context ctx, Item *item, Item_UI *iui, Client *client, UI_Click_State *_close_button_state = NULL, Entity *e = NULL, double world_t = 0)
{
    U(ctx);
    
    Assert(item != NULL);
    Assert(item->type != ITEM_NONE_OR_NUM);

    auto *type = &item_types[item->type];

    User_ID user_id = current_user_id(client);
    bool controls_enabled = (user_id != NO_USER);

    UI_ID window_id;
    Rect window_a = begin_window(&window_id, P(ctx), type->name);
    {
        _AREA_(window_a);

        Entity *player = NULL;
        if(user_id != NO_USER) {
            player = find_player_entity(user_id, &client->room);
        }

        // TABS //
        { _RIGHT_(40); _TRANSLATE_(V2_X * 44);

            bool current_tab_exists = false;
            
            for(int i = 0; i < ITEM_TAB_NONE_OR_NUM; i++)
            {
                auto tab = (Item_Window_Tab)i;

                // Should this tab be shown?
                if(tab == ITEM_TAB_CHESS) {
                    if(item->type != ITEM_CHESS_BOARD || !e || !player) continue;
                }
                //--

                if(i > 0) slide_top(window_default_padding, ctx.layout);

                if(tab == iui->tab) current_tab_exists = true;
                
                { _TOP_SQUARE_SLIDE_();
                    
                    bool enabled  = true;
                    bool selected = iui->tab == tab;
                    
                    if(button(PC(ctx, i), item_window_tab_labels[tab], enabled, selected, opt(C_WINDOW_BORDER_DEFAULT)) & CLICKED_ENABLED)
                        iui->tab = tab;
                }
            }

            if(!current_tab_exists) {
                iui->tab = ITEM_TAB_INFO;
            }
        }
        

        switch(iui->tab)
        {
            case ITEM_TAB_INFO:  { item_info_tab(P(ctx), item, controls_enabled, client, e, player, world_t); } break;
            case ITEM_TAB_CHESS: { item_chess_tab(P(ctx), item, controls_enabled, e, player, world_t, client); } break;
        }
        
    }
    end_window(window_id, ctx.manager, _close_button_state);
}

void room_window(UI_Context ctx, Client *client)
{
    U(ctx);

    float room_button_h = 40;
    
    const int num_rooms = 14;

    Room_ID requested_room = (client->connections.room_connect_requested) ? client->connections.requested_room : -1;
    Room_ID current_room   = client->connections.room.current_room;

    _THEME_(C_THEME_ROOM);
    
    { _WINDOW_(P(ctx), STRING("ROOM"));
        { _TOP_CUT_(room_button_h*2 + window_default_padding);
            _GRID_(num_rooms/2, 2, window_default_padding);
            for(int r = 0; r < num_rooms; r++)
            {
                _CELL_();
                bool selected;
                if(requested_room != -1) selected = (requested_room == r);
                else                     selected = (current_room   == r);

                if(button(PC(ctx, r), concat_tmp("", r), (requested_room == -1), selected) & CLICKED_ENABLED)
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

            textfield(P(ctx), draft, input, enabled);
        }
        cut_left(4, ctx.layout);

        { _LEFT_SQUARE_CUT_();

            bool enabled = (!action_of_type_in_queue(C_RS_ACT_CHAT, client) &&
                            draft->n > 0);
            
            if(button(P(ctx), STRING("SAY"), enabled) & CLICKED_ENABLED)
            {
                C_RS_Action action = {0};
                action.type = C_RS_ACT_CHAT;
                auto &chat = action.chat;

                array_add(client->connections.room_action_queue, action); // @Norelease: IMPORTANT: Any time we want to enqueue a C_RS_Action, we should check that we're connected to a room!
            }
        }
    }
    
    panel(P(ctx));
}

void client_ui(UI_Context ctx, Input_Manager *input, double t, Client *client)
{
    Function_Profile();
    
    U(ctx);

    Client_UI *cui = &client->cui;

    Rect a = area(ctx.layout);


    bool ctrl_is_down  = false;
    bool shift_is_down = false;
    bool alt_is_down   = false;
    for(int i = 0; i < input->keys.n; i++) {
        switch(input->keys.e[i]) {
            case VKEY_CONTROL: ctrl_is_down  = true; break;
            case VKEY_SHIFT:   shift_is_down = true; break;
            case VKEY_ALT:     alt_is_down   = true; break;
        }
    }
    

#if DEVELOPER
    if(cui->dev.window_open)
    { _AREA_COPY_();
        if(cui->user_window_open) cut_right(320 + 64, ctx.layout);
        _RIGHT_(320); _BOTTOM_(480);
        
        client_developer_window(P(ctx), input, &cui->dev, client);
    }
#endif

#if DEBUG
    // PROFILER //
    if(tweak_bool(TWEAK_SHOW_PROFILER)) {
        _BOTTOM_HALF_();
        ui_profiler(P(ctx), PROFILER, input);
    }
#endif
    

    auto *room = &client->room;
    auto world_t = world_time_for_room(room, t);

    Entity *player_entity = NULL;
    User_ID user_id = current_user_id(client);
    if(user_id != NO_USER) {
        player_entity = find_player_entity(user_id, room);
    }

    const float menu_bar_button_s = 52;
    const float menu_bar_pad      = 4;

    // LEFT MENU BAR //
    { _LEFT_CUT_(menu_bar_button_s + menu_bar_pad * 2);

        { _SHRINK_(menu_bar_pad);

            // ROOM BUTTON
            { _TOP_SLIDE_(menu_bar_button_s);
                if(button_colored(P(ctx), C_ROOM_BASE, STRING("ROOM"), true, cui->room_window_open) & CLICKED_ENABLED) {
                    cui->room_window_open = !cui->room_window_open;
                }
            }
            
            slide_top(menu_bar_pad, ctx.layout);

            // USER BUTTON
            { _TOP_SLIDE_(menu_bar_button_s);
                if(button_colored(P(ctx), C_USER_BASE, STRING("USER"), true, cui->user_window_open) & CLICKED_ENABLED) {
                    cui->user_window_open = !cui->user_window_open;
                }
            }
            
            slide_top(menu_bar_pad, ctx.layout);

            // MARKET BUTTON
            { _TOP_SLIDE_(menu_bar_button_s);
                if(button_colored(P(ctx), C_MARKET_BASE, STRING("MARK"), true, cui->market_window_open) & CLICKED_ENABLED) {
                    cui->market_window_open = !cui->market_window_open;
                    if(cui->market_window_open) {
                        reset(&cui->market);
                    }
                }
            }
            
            slide_top(menu_bar_pad, ctx.layout);

#if DEVELOPER
            // DEVELOPER BUTTON
            { _TOP_SLIDE_(menu_bar_button_s);
                if(button(P(ctx), STRING("DEV"), true, cui->dev.window_open) & CLICKED_ENABLED) {
                    cui->dev.window_open = !cui->dev.window_open;
                }
            }
#endif
        }

        panel(P(ctx), opt<v4>({ 0.10, 0.10, 0.10, 1}) );
    }
    

    { _RIGHT_CUT_(64);
        
    }


    // ROOM WINDOW //
    if(cui->room_window_open)
    { _TOP_(window_border_width + window_title_height + window_default_padding + 40 + window_default_padding + 40 + window_default_padding + window_border_width);
        _LEFT_(min(area(ctx.layout).w, 480));
        room_window(P(ctx), client);
    }

    // USER WINDOW //
    if(cui->user_window_open)
    { _RIGHT_CUT_(320);
        user_window(P(ctx), client);
    }

    // MARKET WINDOW//
    if(cui->market_window_open)
    { _CENTER_(640, 480);
        market_window(P(ctx), t, input, &cui->market, client);
    }

    
    { _SHRINK_(16); _LEFT_(240); _BOTTOM_(400);
        
        Item *selected_item = get_selected_inventory_item(&client->user);
        if(selected_item) {
            item_window(P(ctx), selected_item, &cui->item, client);
        }

        
        if(room->selected_entity != NO_ENTITY)
        {
            Entity *e = find_entity(room->selected_entity, room);
            if(e) {
                if(e->type == ENTITY_ITEM)
                {
                    update_entity_item(e, world_t);

                    UI_Click_State close_button_state;
                    item_window(P(ctx), &e->item_e.item, &cui->item, client, &close_button_state, e, world_t);
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

            v3 entity_p      = entity_position(e, world_t, room);
            v2 bottom_center = world_to_screen_space(entity_p + V3_Z * 5, world_view_a, world_projection);

            bottom_center.y = max(chat_y, bottom_center.y);
            if(bottom_center.y > chat_y) chat_y = bottom_center.y;

            // @Robustness: If we change how we draw chat messages we need to change this code that measures the string.
            const Font_Size fs = FS_14;
            const Font_ID font = FONT_BODY;
            
            float max_w = 160;
            Body_Text bt = create_body_text(chat->text, max_w, fs, font, &client->fonts);

            float w = max_w;
            if(bt.lines.n == 1) w = body_text_line_width(0, &bt, &client->fonts);

            Rect a;
            a.s = { w + 8, body_text_height(&bt) + 4 };
            a.x = bottom_center.x - a.w / 2.0f;
            a.y = bottom_center.y - a.h;
            
            chat_y += a.h + 8;

            _AREA_(a);
            ui_chat(PC(ctx, i), chat->text);
        }
        
        // ACTION QUEUE //
        { _RIGHT_CUT_(48);
            if(player_entity != NULL) {
                Assert(player_entity->type == ENTITY_PLAYER);

                auto *player_local = &player_entity->player_local;

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

                            if (target)
                            {
                                Assert(target->type == ENTITY_ITEM); // @Temporary @Norelease: We will have player -> player actions, right?

                                label = item_types[target->item_e.item.type].name;
                                Assert(label.length > 0);
                                label.length = 1;
                            }

                        } break;

                        case PLAYER_ACT_WALK: {
                            label = STRING("WALK");
                        } break;

                        case PLAYER_ACT_PUT_DOWN: {
                            label = STRING("PUT");
                        } break;

                        default: Assert(false); break;
                    }
                    
                    _TOP_SQUARE_CUT_();
                    _SHRINK_(8);
                    bool enabled = false;
                    button(PC(ctx, i), label, enabled);

                    auto *state_after = &player_local->state_after_action_in_queue[i];
                    if(state_after->held_item.type != ITEM_NONE_OR_NUM) {
                        _TRANSLATE_({-48, -24});
                        button(PC(ctx, i), item_types[state_after->held_item.type].name);
                    }
                    
                }
            }
        }
    }
    
    auto *wv = world_view(P(ctx));

    auto *user = current_user(client);
    
    if(user) {
    
        Player_State player_state_after_queue = {0}; // IMPORTANT: Only valid if player != NULL.
        auto *player = find_current_player_entity(client);
        if(player) {
            player_state_after_queue = player_state_after_completed_action_queue(player, world_t, room);
        }
    
        bool ok_to_select_entity = true;

        // Placing Held Item ? //
        room->placing_held_item = ctrl_is_down;
        // // //
    
        // PLACEMENT //
        
        // position
        room->placement_tp = tp_from_index(wv->hovered_tile_ix);

        bool ignore_surfaces = alt_is_down;
        
        Surface hovered_surface;
        if(!ignore_surfaces && get(wv->hovered_surface, &hovered_surface)) {
            if(hovered_surface.flags & SURF_CENTERING) {
                room->placement_tp   = hovered_surface.p + V3(hovered_surface.s) * .5;
            } else {
                room->placement_tp   = tp_from_p(wv->hovered_surface_hit_p);
                room->placement_tp.z = wv->hovered_surface_hit_p.z;
            }
        }

        // rotation
        if(!floats_equal(magnitude(room->placement_q), 1.0f))
            room->placement_q = Q_IDENTITY; // placement_q will be zero the first time, since Rooms are zero-initialized.
            
        if(in_array(input->keys_down, VKEY_z))
            room->placement_q *= axis_rotation(V3_Z, -0.25f * TAU);
        //--
            
        // // //

        Item *item_to_place = NULL;
        
        if(room->placing_held_item) {
            if(player_state_after_queue.held_item.type != ITEM_NONE_OR_NUM) {
                item_to_place = &player_state_after_queue.held_item;
            }
        }
        else item_to_place = get_selected_inventory_item(user);
    
        if(item_to_place) {

            // PLACE FROM INVENTORY OR PUT DOWN //

            if(wv->click_state & CLICKED_ENABLED)
            {
                v3 tp = room->placement_tp;
                Quat q = room->placement_q;

                if (can_place_item_entity_at_tp(item_to_place, tp, q, world_t, room))
                {
                    if(room->placing_held_item) {
                        Player_Action action = {0};
                        action.type = PLAYER_ACT_PUT_DOWN;
                        action.put_down.tp = tp;
                        action.put_down.q  = q;

                        request(&action, client);
                        
                    } else {
                        Player_Action action = {0};
                        action.type = PLAYER_ACT_PLACE_FROM_INVENTORY;
                        action.place_from_inventory.item = item_to_place->id;
                        action.place_from_inventory.tp   = tp;
                        action.place_from_inventory.q    = q;
                
                        request(&action, client);

                        Entity preview_entity = create_preview_item_entity(item_to_place, tp, q, world_t);

                        // NOTE: We add a preview entity and remove the placed item locally, before
                        //       we know if the operation succeeds.
                        //       We assumes the servers will send us a ROOM_UPDATE / USER_UPDATE on
                        //       transaction abort so we get our stuff back / know when to remove the
                        //       preview entity.

                        auto type = item_to_place->type;

                        add_entity(preview_entity, room);
                        inventory_remove_item_locally(item_to_place->id, user);

                        if (!shift_is_down ||
                            !select_next_inventory_item_of_type(type, user))
                        {
                            inventory_deselect(user);
                        }
                    }
                }
            }
        
            ok_to_select_entity = false;
        }
        else if(wv->clicked_tile_ix >= 0 && wv->hovered_entity == NO_ENTITY) {
        
            // WALK //
        
            C_RS_Action action = {0};
            action.type = C_RS_ACT_CLICK_TILE;
            auto &ct = action.click_tile;
            ct.tile_ix = wv->clicked_tile_ix;

            // @Norelease: IMPORTANT: Any time we want to enqueue a C_RS_Action, we should check that we're connected to a room!
            array_add(client->connections.room_action_queue, &action);
            
            ok_to_select_entity = false;
        }

        // CLICK ENTITY //
        if(wv->clicked_entity != NO_ENTITY) {
    
            if(ok_to_select_entity)
            {
                // SELECT ENTITY //
                client->room.selected_entity = wv->clicked_entity;
            }
        }
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
    
    input.mouse.p = { (float)x, client->main_window_a.h - (float)y };
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

void update_client(Client *client, Input_Manager *input)
{
    User *user = current_user(client);
    
    // ESCAPE KEY //
    if(in_array(input->keys_down, VKEY_ESCAPE)) {
        
        if(client->user.selected_inventory_item_plus_one > 0) {
            inventory_deselect(&client->user);
        }
    }

    // MAKE SURE WE'RE CONNECTED TO MARKET //
    // Maybe we only should try to connect to the market server
    // if the window is open, but for now keep it clean and just
    // be connected always. @Norelease
    if(user) {
        bool connected  = client->connections.market.connected;
        bool connecting = client->connections.market_connect_requested;
        
        if(!connected && !connecting) {
            // @Norelease: reconnect timeout!
            request_connection_to_market(client);
        }
    }


    // @Cleanup: This is a thing to find random paths (for profiling)
    #if 0
    {
        v3 p1s[] = {
            { (float)random_int(0, room_size_x-1), (float)random_int(0, room_size_y-1), 0 },
            { (float)random_int(0, room_size_x-1), (float)random_int(0, room_size_y-1), 0 },
            { (float)random_int(0, room_size_x-1), (float)random_int(0, room_size_y-1), 0 },
            { (float)random_int(0, room_size_x-1), (float)random_int(0, room_size_y-1), 0 },
            { (float)random_int(0, room_size_x-1), (float)random_int(0, room_size_y-1), 0 },
            { (float)random_int(0, room_size_x-1), (float)random_int(0, room_size_y-1), 0 },
            { (float)random_int(0, room_size_x-1), (float)random_int(0, room_size_y-1), 0 }
        };

        Array<v3, ALLOC_TMP> path = {0};
        double dur;
        if(find_path_to_any({12, 12, 0}, p1s, ARRLEN(p1s), &client->room.walk_map, true, &path, &dur)) {
            Entity *e = find_current_player_entity(client);
            if(e)
            {
                if(e->player_e.walk_path)
                    dealloc(e->player_e.walk_path, ALLOC_MALLOC);
                e->player_e.walk_path = (v3 *)alloc(sizeof(v3) * path.n, ALLOC_MALLOC);
                memcpy(e->player_e.walk_path, path.e, sizeof(v3) * path.n);
                e->player_e.walk_path_length = path.n;
            }
        }
    }
    #endif
    
}

int client_entry_point(int num_args, char **arguments)
{                             
    platform_init_socket_use();
    
    Debug_Print("I am a client.\n");
    
    // INIT CLIENT //
    Client client = {0};
    create_mutex(client.mutex);
    clear_and_reset(&client.market);
    // /////////// //
    
    Layout_Manager *layout = &client.layout;
    UI_Manager     *ui = &client.ui;
    Input_Manager  *input = &client.input;
    Window *main_window = &client.main_window;
    //--

    // @Norelease: Doing Developer stuff in release build...
    init_developer(&client.developer);
    load_tweaks(client.developer.user_id);
#if OS_WINDOWS
    setup_tweak_hotloading();
#endif

    // CREATE WINDOW //
    v4s window_rect = tweak_v4s(TWEAK_INITIAL_OS_WINDOW_RECT);
    platform_create_window(main_window, "Citrus", window_rect.w, window_rect.h, window_rect.x, window_rect.y);
    client_set_window_delegate(main_window, &client);
    platform_get_window_rect(main_window, &client.main_window_a.x,  &client.main_window_a.y,  &client.main_window_a.w,  &client.main_window_a.h);
    //--

    // LOAD ASSETS //
    load_assets(&client.assets);
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



#if DEVELOPER
    // AUTO-CONNECT TO STARTUP USER/ROOM? //
    auto startup_user = tweak_uint(TWEAK_STARTUP_USER);
    if(startup_user > 0) {
        
        lock_mutex(client.mutex);
        {
            request_connection_to_user(startup_user, &client);
        }
        unlock_mutex(client.mutex);
        
        auto startup_room = tweak_uint(TWEAK_STARTUP_ROOM);
        if(startup_room > 0) {

            bool user_connect_successful;
            
            // Wait for connection to user
            while(true) {

                platform_sleep_milliseconds(1);
                //--
                
                lock_mutex(client.mutex);
                defer(unlock_mutex(client.mutex););
                    
                auto *cons = &client.connections;
                if(!cons->user_connect_requested) {
                    user_connect_successful = cons->user.connected;
                    break;
                }
            }

            if(user_connect_successful) 
                request_connection_to_room(startup_room, &client);
            else
                Debug_Print("Not trying to connect to startup room, because connecting to startup user failed.\n");
        }
        
    }
    //--
#endif
    
   
    while(true)
    {
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

        Cursor_Icon cursor;

        int num_input_messages_received = 0;
        
        { Scoped_Lock(client.mutex);
            
            // MAYBE RELOAD TWEAKS //
#if OS_WINDOWS
            {
                bool old_color_tiles_by_position = tweak_bool(TWEAK_COLOR_TILES_BY_POSITION);
                {
                    maybe_reload_tweaks(client.developer.user_id);
                }
                bool new_color_tiles_by_position = tweak_bool(TWEAK_COLOR_TILES_BY_POSITION);

#if DEBUG
                if(new_color_tiles_by_position != old_color_tiles_by_position) {
                    client.room.static_geometry_up_to_date = false;
                }
#endif
            }
#endif
            // ------------------ //


            bool should_quit = false;
            int num_ui_loops = 0;
        
            while(true) {
                // RECEIVE NEXT INPUT MESSAGE //
                new_input_frame(input);
                bool any_input_message = platform_receive_next_input_message(main_window, &should_quit);

                if(should_quit) break;
                if(!any_input_message && num_ui_loops > 0) break;
                
                if(any_input_message) num_input_messages_received++;
                num_ui_loops++;
                ////////////////////////////////
                
                defer(next_profiler_frame(PROFILER););
                Scoped_Profile("Update");
                //--

                client.main_window_a = window_a;
                
                update_client(&client, input);
            
                // BUILD UI //
                begin_ui_build(ui);
                {
                    push_area_layout(window_a, layout);
            
                    client_ui(P(ui_ctx), input, t, &client);
            
                    pop_layout(layout);
                }
                    
                double t = platform_get_time(); // @Robustness: This is safe to do, right?
                end_ui_build(ui, &client.input, &client.fonts, t, &client.room, &client.assets, &cursor);
                // //////// //

                reset_temporary_memory();
                
            }

            
#if DEBUG || true
            if(second != last_second) {
                ups = updates_this_second;

                char *title = concat_cstring_tmp("Citrus | ", fps, " FPS | ", ups, " UPS | ", draw_calls_last_frame, " draws | ");
                title = concat_cstring_tmp(title, triangles_this_frame, " tris");
                
                platform_set_window_title(main_window, title);
                    
                updates_this_second = 0;
            }
            updates_this_second++;
#endif
            
            if(should_quit) break;
            
        }

        if(num_input_messages_received == 0) platform_sleep_milliseconds(2);

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
