
void item_substance_container_ui(UI_Context ctx, Substance_Container *c, Item *item, bool controls_enabled, double t, Client *client, Entity *e = NULL, Player_State *player_state_after_queue = NULL, double world_t = 0, Room *room = NULL);


void set_current_tool(Tool_ID tool, Client_UI *cui, double t)
{
    if(cui->current_tool == tool) return;
    
    cui->last_tool = cui->current_tool;
    cui->current_tool = tool;
    cui->tool_switch_t = t;

    switch(tool) {
        case TOOL_PLANTING: {
            cui->planting_tool.seed_container  = NO_ENTITY;
            cui->planting_tool.seed_type_cache = SEED_NONE_OR_NUM;
        } break;

#if DEVELOPER
        case TOOL_DEV_ROOM_EDITOR: {
            reset_room_editor(&cui->room_editor);
        } break;
#endif
    }
}


void update_entity_menu(Entity_Menu *menu, double t, UI_World_View *wv, Client_UI *cui)
{
    if(wv->click_state & LEFT_CLICKED_ENABLED || wv->click_state & RIGHT_CLICKED_ENABLED)
    {
        set_current_tool(TOOL_NONE, cui, t);
    }
}

Tool_Stay_Open_Signal entity_menu_ui(UI_Context ctx, Entity_Menu *menu, bool is_current_tool,
                                     double t, Player_State *player_state_after_queue,
                                     double world_t, Room *room, Client *client)
{
    U(ctx);

    auto *cui   = &client->cui;

    Tool_Stay_Open_Signal stay_open = TOOL_STAY_OPEN;
    
    Entity *e = find_entity(menu->entity, room);
    if(!e) return TOOL_CLOSE_INSTANTLY;

    float anim_p = action_menu_anim_p(cui->tool_switch_t, t, is_current_tool);

    if(anim_p <= 0) return stay_open;

    
    Assert(e->type == ENTITY_ITEM);
    Item      *item = &e->item_e.item;
    Item_Type *type = &item_types[item->type];
    
    if(type->container_forms) {
        if(item->container.amount > 0) {
            auto *container = &item->container;
            auto *substance = &container->substance;
            
            Rect aa = { menu->p + V2_Y * 20, { 120, 42 } };
            aa.x -= aa.w * .5f;
            _AREA_(aa); _BOTTOM_(aa.h * anim_p);
            item_substance_container_ui(P(ctx), container, item, true, t, client, e, player_state_after_queue, world_t, room); // @Norelease @Hardcoded 'enabled' value
        }
    }


    Array<Entity_Action, ALLOC_TMP> actions = {0};
    bool first_action_is_default;
    get_available_actions_for_entity(e, player_state_after_queue, &actions, &first_action_is_default);

    {
        Rect aa = {
            menu->p,
            { 100, action_menu_height(actions.n, anim_p) }
        };
        aa.x -= aa.w * .5f;

        float hh = aa.h / actions.n; // Current height of an action item.
        
        aa.y -= aa.h - hh * .55;
                    
        if(!first_action_is_default) aa.y -= hh; // Don't place first action under mouse cursor if it's not considered the 'default action'

        _AREA_(aa);
                    
        _TOP_(hh);
        for(int i = 0; i < actions.n; i++) {
            auto &act = actions[i];
            String label = entity_action_label(act);
            bool enabled = predicted_possible(&act, e->id, world_t, *player_state_after_queue, client);
                        
            if(button(PC(ctx, i), label, enabled) & CLICKED_ENABLED) {
                // Request action
                request(&act, e->id, client);
                            
                stay_open = TOOL_CLOSE_INSTANTLY;
            }
                        
            slide_top(hh, ctx.layout);
        }
    }


    return stay_open;
}


bool start_planting_predicted_possible(Entity *seeds_container, double world_t, Player_State *player_state_after_queue, Client *client)
{
    Assert(seeds_container->type == ENTITY_ITEM);
    
    if(player_state_after_queue->held_item.type != ITEM_NONE_OR_NUM &&
       player_state_after_queue->held_item.id   == seeds_container->item_e.item.id)
        return true;
    
    Entity_Action act = {0};
    act.type = ENTITY_ACT_PICK_UP;
    return predicted_possible(&act, seeds_container->id, world_t, *player_state_after_queue, client);
}

// NOTE: Call start_planting_predicted_possible() before calling this.
void start_planting(Entity *seeds_container, Entity *player, double t, Client_UI *cui, Client *client)
{
    Entity_Action act = {0};
    act.type = ENTITY_ACT_PICK_UP;

    set_current_tool(TOOL_PLANTING, cui, t);
    cui->planting_tool.seed_container = seeds_container->id;
    
    auto *ps = &player->player_local.state_before_action_in_queue[player->player_e.action_queue_length];
    if(ps->held_item.type == ITEM_NONE_OR_NUM) {
        auto *requested = request(&act, seeds_container->id, client); // @Incomplete @Robustness: What happens if this fails?
        requested->is_pick_up_for_planting = true;
        cui->planting_tool.waiting_for_pickup_enqueue = true;
    } else {
        Assert(ps->held_item.id == seeds_container->item_e.item.id);
        cui->planting_tool.waiting_for_pickup_enqueue = false;
    }
    
}

void update_planting_tool(Planting_Tool *tool, double t, UI_World_View *wv, Client_UI *cui,
                          Player_State *player_state_after_queue,
                          double world_t, Room *room, Client *client)
{
    auto *player = find_current_player_entity(client);
    if(!player) return;
    
    if(tool->waiting_for_pickup_enqueue) return;

    if(tool->seed_container != NO_ENTITY) {
        auto *e = find_entity(tool->seed_container, room);
        if(!e) return;
        
        Entity_Action act = {0};
        act.type = ENTITY_ACT_PLANT;
        act.plant.tp = tp_from_index(wv->hovered_tile_ix);
        
        if(wv->click_state & CLICKED_ENABLED) {
            request_if_predicted_possible(&act, e->id, world_t, *player_state_after_queue, client);
        }
    }
    else {
        if(wv->left_clicked_entity != NO_ENTITY) {
            auto *e = find_entity(wv->left_clicked_entity, room);
            Assert(e);

            if(e->type == ENTITY_ITEM && item_types[e->item_e.item.type].container_forms & SUBST_NUGGET) {
                update_entity_item(e, world_t);
                if(nugget_type_of_container(&e->item_e.item.container) == NUGGET_SEEDS)
                {
                    if(start_planting_predicted_possible(e, world_t, player_state_after_queue, client)) {
                        start_planting(e, player, t, cui, client);
                    }
                }
            }
        }
    }
}

Tool_Stay_Open_Signal planting_tool_ui(UI_Context ctx, Planting_Tool *tool, bool is_current_tool,
                                       double t, Client_UI *cui, Player_State *player_state_after_queue,
                                       double world_t, Room *room, Client *client)
{
    // TODO @Norelease: Show to the user that we are waiting_for_pickup_enqueue -- and that we can't do anything until we're not.
    
    U(ctx);
    
    tool->seed_type_cache = SEED_NONE_OR_NUM;

    auto stay_open = TOOL_STAY_OPEN;

    float anim_p = clamp(t - cui->tool_switch_t);
    if(!is_current_tool) anim_p = 0;

    if(anim_p <= 0) return stay_open;

    Entity *e = NULL;
    Item   *item = NULL;
    Substance_Container *container = NULL;
    
    if(tool->seed_container != NO_ENTITY) {
        e = find_entity(tool->seed_container, room);
        if(!e) {
            tool->seed_container = NO_ENTITY;
        } else {
            Assert(e->type == ENTITY_ITEM);
            Assert(item_types[e->item_e.item.type].container_forms & SUBST_NUGGET);

            update_entity_item(e, world_t);
            item      = &e->item_e.item;
            container = &item->container;
            if(container->substance.form != SUBST_NUGGET)           return TOOL_CLOSE_INSTANTLY;
            if(nugget_type_of_container(container) != NUGGET_SEEDS) return TOOL_CLOSE_INSTANTLY;

            if(!tool->waiting_for_pickup_enqueue &&
               (player_state_after_queue->held_item.type == ITEM_NONE_OR_NUM || // @Incomplete @Volatile: Make held_item an Optional or something.
                player_state_after_queue->held_item.id != item->id))
            {
                // @Incomplete Break();
                return TOOL_CLOSE_INSTANTLY;
            }

            auto seed = container->substance.nugget.seed_type;
            tool->seed_type_cache = seed;
        }
    }

    { _CENTER_(200, 140);

        _WINDOW_(P(ctx), STRING("PLANTING"));
        /*
          @Incomplete NOTE:
          We do some ambitious stuff here just to try out how an in-depth UI might
          be like.

          DONT SKIP ANY OF THIS!!!!
        */

        if(e) {
            Assert(item && container);
            
            { _LEFT_CUT_(42);
                _TOP_SQUARE_();
                ui_item_image(P(ctx), item->type);
            }

            ui_text(P(ctx), concat_tmp(item->id.origin, ":", item->id.number));

            // @Incomplete TODO: Reserve space where container was before pick-up.
            //           TODO: Put container back when we click Done or switch to another tool.
            
            // @Incomplete TODO: Show item
            // @Incomplete TODO: Show container
            // @Incomplete TODO: Show icon for each position.
            // @Incomplete TODO: Highlight position in world when hovering a position icon
            // @Incomplete TODO: Remove position when clicking its icon.
            // @Incomplete TODO: Add PLAYER_ACT_PLANT (container, tile_ix).
            // @Incomplete TODO: Figure out actions on "Done".
            /*                 - if not holding container, add pick-up action
                               - for each position, add plant action
            */
            // @Incomplete TODO: Check that all actions are possible.
            /*
              - if a plant action is not possible, make its icon red.
              - when hovering a red position icon, show TOOLTIP (Add tooltip String variable to UI_Manager)
              that says what the reason is (Add REASONS to prediction infos)
              - if a pick-up action is not possible, show icons for reasons on "Done" button.
            */
            // @Incomplete TODO: Make it possible to send multiple actions with RSB_PLAYER_ACTION_ENQUEUE.
            // @Incomplete TODO: Enqueue all actions on "Done".
            // @Incomplete TODO: Show text if at max positions without overflowing queue.
            // @Incomplete TODO: Show text and disable "Done" button if queue would overflow.
        
            _BOTTOM_(32);
            if(button(P(ctx), STRING("DONE")) & CLICKED_ENABLED) {
                stay_open = TOOL_CLOSE_ANIMATED;
            }
        } else {
            ui_text(P(ctx), STRING("Choose a seeds container to start planting."));
        }
    }
    
    return stay_open;
}
