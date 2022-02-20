
void init_ui_dock(UI_Dock *dock)
{
    UI_Dock_Section root_section;
    Zero(root_section);
    
    array_add(dock->sections, root_section);
}


// NOTE: Returns a split mode other than NONE if a split was requested.
UI_Dock_Split_Mode ui_dock_section_top_bar(UI_Context ctx, UI_Dock_Section *section, UI_Dock_Section *parent, UI_Dock *dock, bool *_empty_view_requested, bool *_close_requested)
{
    U(ctx);

    *_close_requested = false;
    *_empty_view_requested = false;

    Assert(section->split == UI_DOCK_SPLIT_NONE);
    String title = title_for_view(&section->view);

    UI_Dock_Split_Mode requested_split = UI_DOCK_SPLIT_NONE;

    if(parent)
    { _RIGHT_SQUARE_CUT_();
        if(button(P(ctx), STRING("X")) & CLICKED_ENABLED) {
            *_close_requested = true;
        }
    }

    if(section->view.address.type != VIEW_NONE)
    { _RIGHT_SQUARE_CUT_();
        if(button(P(ctx), STRING("/")) & CLICKED_ENABLED) {
            *_empty_view_requested = true;
        }
    }
    
    { _RIGHT_SQUARE_CUT_();
        if(button(P(ctx), STRING("|")) & CLICKED_ENABLED) {
            requested_split = UI_DOCK_SPLIT_HORIZONTAL;
        }
    }
    { _RIGHT_SQUARE_CUT_();
        if(button(P(ctx), STRING("-")) & CLICKED_ENABLED) {
            requested_split = UI_DOCK_SPLIT_VERTICAL;
        }
    }
    button(P(ctx), title);

    return requested_split;
}


UI_Dock_Section *create_ui_dock_section(UI_Dock *dock, int *_ix)
{
    UI_Dock_Section *section = NULL;
    
    if(dock->free_section_slots.n > 0) {
        *_ix = last_element(dock->free_section_slots);
        dock->free_section_slots.n--;
        section = &dock->sections[*_ix];
    } else {
        *_ix = dock->sections.n;
        section = array_add_uninitialized(dock->sections);
    }

    Zero(*section);
    return section;
}

void split_ui_dock_section(UI_Dock_Section *section, UI_Dock_Split_Mode split, UI_Dock *dock)
{
    Assert(split != UI_DOCK_SPLIT_NONE);
    Assert(section->split == UI_DOCK_SPLIT_NONE);

    View view = section->view;
    section->split_percentage = 50;

    Zero(section->sub_sections);
    
    int section_ix = section - dock->sections.e; // Because pointer might be invalid after adding to array.

    for(int i = 0; i < ARRLEN(section->sub_sections); i++) {
        bool empty_section = ((i == 1) == (split == UI_DOCK_SPLIT_HORIZONTAL));

        int ix;
        UI_Dock_Section *sub_section = create_ui_dock_section(dock, &ix);
        section = &dock->sections[section_ix]; // Because pointer might be invalid after adding to array.
        
        if(!empty_section) {
            sub_section->view = view;
        }

        section->sub_sections[i] = ix;
    }

    section->split = split;
}


void merge_split_ui_dock_section(UI_Dock_Section *section, int sub_section_to_close, UI_Dock *dock, Client *client)
{
    Assert(section->split != UI_DOCK_SPLIT_NONE);

    auto sub_to_keep_ix  = section->sub_sections[(sub_section_to_close == 0) ? 1 : 0];
    auto sub_to_close_ix = section->sub_sections[sub_section_to_close];
    
    UI_Dock_Section *sub_to_keep  = &dock->sections[sub_to_keep_ix];
    UI_Dock_Section *sub_to_close = &dock->sections[sub_to_close_ix];

    *section = *sub_to_keep; // Replace parent with the sub to keep

    Assert(sub_to_close->split == UI_DOCK_SPLIT_NONE);
    pre_view_close(&sub_to_close->view, client);
    clear(&sub_to_close->view);
    
    array_add(dock->free_section_slots, sub_to_keep_ix);
    array_add(dock->free_section_slots, sub_to_close_ix);
}

void ui_dock_section(UI_Context ctx, UI_Dock_Section *section, UI_Dock_Section *parent, UI_Dock *dock, Input_Manager *input, bool *_empty_view_requested, bool *_close_requested)
{
    U(ctx);

    *_empty_view_requested = false;
    *_close_requested = false;

    // VIEW
    if(section->split == UI_DOCK_SPLIT_NONE) {
        UI_Dock_Split_Mode requested_split = UI_DOCK_SPLIT_NONE;
        { _TOP_CUT_(24); requested_split = ui_dock_section_top_bar(P(ctx), section, parent, dock, _empty_view_requested, _close_requested); }

        { _PANEL_(P(ctx), opt<v4>({0.16,0.22,0.23,1}));
            view(P(ctx), &section->view, input);
        }

        if(requested_split != UI_DOCK_SPLIT_NONE) {
            split_ui_dock_section(section, requested_split, dock);
        }
        
        return;
    }

    // SPLIT
    float fraction = section->split_percentage * .01;
    Rect a = area(ctx.layout);

    UI_Dock_Section *sub_section_1 = &dock->sections[section->sub_sections[0]];
    UI_Dock_Section *sub_section_2 = &dock->sections[section->sub_sections[1]];

    UI_Click_State slider_click_state;

    int close_requester = -1;
    int empty_view_requester = -1;
    
    bool close_requested, empty_view_requested; // Reused for all sub sections

    switch(section->split) {
        case UI_DOCK_SPLIT_HORIZONTAL: {

            // SLIDER
            { _LEFT_(8); _TRANSLATE_(V2_X * (a.w * fraction - 4));
                slider_click_state = button(P(ctx), EMPTY_STRING, true, false, {0}, UI_BUTTON_STYLE_INVISIBLE, CURSOR_ICON_RESIZE_H);
            }

            // LEFT SUB SECTION
            { _LEFT_CUT_(a.w * fraction);
                ui_dock_section(P(ctx), sub_section_1, section, dock, input, &empty_view_requested, &close_requested);
                if(close_requested)      close_requester = 0;
                if(empty_view_requested) empty_view_requester = 0;
            }
            
            // RIGHT SUB SECTION
            ui_dock_section(P(ctx), sub_section_2, section, dock, input, &empty_view_requested, &close_requested);
            if(close_requested)      close_requester = 1;
            if(empty_view_requested) empty_view_requester = 1;
        } break;

        case UI_DOCK_SPLIT_VERTICAL: {

            // SLIDER
            { _BOTTOM_(8); _TRANSLATE_(V2_Y * (a.h * fraction - 4));
                slider_click_state = button(P(ctx), EMPTY_STRING, true, false, {0}, UI_BUTTON_STYLE_INVISIBLE, CURSOR_ICON_RESIZE_V);
            }

            // BOTTOM SUB SECTION
            { _BOTTOM_CUT_(a.h * fraction);
                ui_dock_section(P(ctx), sub_section_1, section, dock, input, &empty_view_requested, &close_requested);
                if(close_requested)      close_requester = 0;
                if(empty_view_requested) empty_view_requester = 0;
            }

            // TOP SUB SECTION
            ui_dock_section(P(ctx), sub_section_2, section, dock, input, &empty_view_requested, &close_requested);
            if(close_requested)      close_requester = 1;
            if(empty_view_requested) empty_view_requester = 1;
        } break;

        default: Assert(false); break;
    }

    if(close_requester >= 0) {
        merge_split_ui_dock_section(section, close_requester, dock, ctx.client);
        return;
    }

    if(empty_view_requester >= 0) {
        auto *sub = &dock->sections[section->sub_sections[empty_view_requester]];
        Assert(sub->split == UI_DOCK_SPLIT_NONE);
        Assert(sub->view.address.type != VIEW_NONE);
        View empty_view;
        Zero(empty_view);
        replace_view(&sub->view, &empty_view, ctx.client);
    }

    int slider_comp = (section->split == UI_DOCK_SPLIT_VERTICAL) ? 1 : 0;

    if(slider_click_state & PRESSED_NOW){
        section->split_slider_mouse_drag_offset = input->mouse.p.comp[slider_comp] - (a.p.comp[slider_comp] + a.s.comp[slider_comp] * fraction);
    }

    if(slider_click_state & PRESSED) {
        float fraction_under_mouse = (input->mouse.p.comp[slider_comp] + section->split_slider_mouse_drag_offset - a.p.comp[slider_comp]) / a.s.comp[slider_comp];
        section->split_percentage =  clamp<int>(fraction_under_mouse * 100, 20, 80);
    }

    return;
}


void ui_dock(UI_Context ctx, UI_Dock *dock, Input_Manager *input)
{
    U(ctx);

    auto *root = &dock->sections[0];
    
    Assert(dock->sections.n > 0);
    bool close_requested; 
    bool empty_view_requested;
    ui_dock_section(P(ctx), root, NULL, dock, input, &empty_view_requested, &close_requested);

    if(empty_view_requested) {
        Assert(root->split == UI_DOCK_SPLIT_NONE);
        Assert(root->view.address.type != VIEW_NONE);
        View empty_view;
        Zero(empty_view);
        replace_view(&root->view, &empty_view, ctx.client);
    }
}
