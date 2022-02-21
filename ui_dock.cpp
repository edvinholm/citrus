
void init_ui_dock(UI_Dock *dock)
{
    UI_Dock_Section root_section;
    Zero(root_section);
    
    array_add(dock->sections, root_section);
}


// NOTE: Returns a split mode other than NONE if a split was requested.
UI_Dock_Split_Mode ui_dock_section_top_bar(UI_Context ctx, UI_Dock_Section *section, UI_Dock_Section *parent, UI_Dock *dock, bool *_empty_view_requested, bool *_close_requested, bool *_tab_requested)
{
    U(ctx);

    *_close_requested = false;
    *_empty_view_requested = false;
    *_tab_requested = false;

    Assert(section->split == UI_DOCK_SPLIT_NONE);
    String title = title_for_view(&section->view);

    UI_Dock_Split_Mode requested_split = UI_DOCK_SPLIT_NONE;

    { _LEFT_SQUARE_CUT_(); // Add tab
        if(button(P(ctx), STRING("+")) & CLICKED_ENABLED) {
            *_tab_requested = true;
        }
    }
    
    if(parent)
    { _RIGHT_SQUARE_CUT_();
        if(button(P(ctx), STRING("X")) & CLICKED_ENABLED) { // Close
            *_close_requested = true;
        }
    }

    if(section->view.address.type != VIEW_EMPTY)
    { _RIGHT_SQUARE_CUT_();
        if(button(P(ctx), STRING("/")) & CLICKED_ENABLED) { // Empty view
            *_empty_view_requested = true;
        }
    }
    
    { _RIGHT_SQUARE_CUT_();
        if(button(P(ctx), STRING("|")) & CLICKED_ENABLED) { // Split horizontally
            requested_split = UI_DOCK_SPLIT_HORIZONTAL;
        }
    }
    { _RIGHT_SQUARE_CUT_();
        if(button(P(ctx), STRING("-")) & CLICKED_ENABLED) { // Split vertically
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
    Assert(section->split == UI_DOCK_SPLIT_NONE || section->split == UI_DOCK_SPLIT_TABBED);

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

    dock->unsaved_changes = true;
}


void merge_split_ui_dock_section(UI_Dock_Section *section, int sub_section_to_close, UI_Dock *dock, Client *client)
{
    Assert(section->split != UI_DOCK_SPLIT_NONE && section->split != UI_DOCK_SPLIT_TABBED);

    auto sub_to_keep_ix  = section->sub_sections[(sub_section_to_close == 0) ? 1 : 0];
    auto sub_to_close_ix = section->sub_sections[sub_section_to_close];
    
    UI_Dock_Section *sub_to_keep  = &dock->sections[sub_to_keep_ix];
    UI_Dock_Section *sub_to_close = &dock->sections[sub_to_close_ix];

    *section = *sub_to_keep; // Replace parent with the sub to keep

    Assert(sub_to_close->split == UI_DOCK_SPLIT_NONE || sub_to_close->split == UI_DOCK_SPLIT_TABBED);
    pre_view_close(&sub_to_close->view, client);
    clear(&sub_to_close->view);
    
    array_add(dock->free_section_slots, sub_to_keep_ix);
    array_add(dock->free_section_slots, sub_to_close_ix);

    dock->unsaved_changes = true;
}


// NOTE: May modify *<x>_requester_
void ui_dock_section(UI_Context ctx, UI_Dock_Section *section, UI_Dock_Section *parent, UI_Dock *dock, Input_Manager *input, int *empty_view_requester_, int *close_requester_, int *tab_requester_)
{
    U(ctx);

    int section_ix = section - dock->sections.e; // IMPORTANT: After doing anything that can result in a reallocate dock->sections, we need to use these indices to refresh the pointers to the sections.
    int parent_ix  = (parent) ? parent - dock->sections.e : -1;

    // VIEW
    if(section->split == UI_DOCK_SPLIT_NONE) {
        bool empty_view_requested, close_requested, tab_requested;

        UI_Dock_Split_Mode requested_split = UI_DOCK_SPLIT_NONE;
        { _TOP_CUT_(24); requested_split = ui_dock_section_top_bar(P(ctx), section, parent, dock, &empty_view_requested, &close_requested, &tab_requested); }

        if(empty_view_requested) *empty_view_requester_ = section_ix;
        if(close_requested)      *close_requester_      = section_ix;
        if(tab_requested)        *tab_requester_        = section_ix;
        
        { _PANEL_(P(ctx));
            view(P(ctx), &section->view, input);
        }

        if(requested_split != UI_DOCK_SPLIT_NONE) {
            split_ui_dock_section(section, requested_split, dock);
            return; // IMPORTANT: If we don't want to return here we should refresh our section pointers before continuing.
        }
        
        return;
    }

    // SPLIT
    float fraction = section->split_percentage * .01;
    Rect a = area(ctx.layout);

    UI_Click_State slider_click_state;
    
    switch(section->split) {
        case UI_DOCK_SPLIT_HORIZONTAL: {

            // SLIDER
            { _LEFT_(8); _TRANSLATE_(V2_X * (a.w * fraction - 4));
                slider_click_state = button(P(ctx), EMPTY_STRING, true, false, {0}, UI_BUTTON_STYLE_INVISIBLE, 0, CURSOR_ICON_RESIZE_H);
            }

            // LEFT SUB SECTION
            { _LEFT_CUT_(a.w * fraction);
                UI_Dock_Section *sub_section_1 = &dock->sections[section->sub_sections[0]];
                ui_dock_section(P(ctx), sub_section_1, section, dock, input, empty_view_requester_, close_requester_, tab_requester_);

                // Refreshing pointers.
                section = &dock->sections[section_ix];
                parent  = (parent_ix >= 0) ? &dock->sections[parent_ix] : NULL;
            }
            
            // RIGHT SUB SECTION
            UI_Dock_Section *sub_section_2 = &dock->sections[section->sub_sections[1]];
            ui_dock_section(P(ctx), sub_section_2, section, dock, input, empty_view_requester_, close_requester_, tab_requester_);

            // Refreshing pointers.
            section = &dock->sections[section_ix];
            parent  = (parent_ix >= 0) ? &dock->sections[parent_ix] : NULL;
        } break;

        case UI_DOCK_SPLIT_VERTICAL: {

            // SLIDER
            { _BOTTOM_(8); _TRANSLATE_(V2_Y * (a.h * fraction - 4));
                slider_click_state = button(P(ctx), EMPTY_STRING, true, false, {0}, UI_BUTTON_STYLE_INVISIBLE, 0, CURSOR_ICON_RESIZE_V);
            }

            // BOTTOM SUB SECTION
            { _BOTTOM_CUT_(a.h * fraction);
                UI_Dock_Section *sub_section_1 = &dock->sections[section->sub_sections[0]];
                ui_dock_section(P(ctx), sub_section_1, section, dock, input, empty_view_requester_, close_requester_, tab_requester_);

                // Refreshing pointers.
                section = &dock->sections[section_ix];
                parent  = (parent_ix >= 0) ? &dock->sections[parent_ix] : NULL;
            }

            // TOP SUB SECTION
            UI_Dock_Section *sub_section_2 = &dock->sections[section->sub_sections[1]];
            ui_dock_section(P(ctx), sub_section_2, section, dock, input, empty_view_requester_, close_requester_, tab_requester_);

            // Refreshing pointers.
            section = &dock->sections[section_ix];
            parent  = (parent_ix >= 0) ? &dock->sections[parent_ix] : NULL;
        } break;

        case UI_DOCK_SPLIT_TABBED: {

            int visible_sub_section = 0; // @Incomplete
            
            UI_Dock_Section *sub_section = &dock->sections[section->sub_sections[visible_sub_section]];
            ui_dock_section(P(ctx), sub_section, section, dock, input, empty_view_requester_, close_requester_, tab_requester_);

            // Refreshing pointers.
            section = &dock->sections[section_ix];
            parent  = (parent_ix >= 0) ? &dock->sections[parent_ix] : NULL;
            
        } break;

        default: Assert(false); break;
    }
    
    int slider_comp = (section->split == UI_DOCK_SPLIT_VERTICAL) ? 1 : 0;

    if(slider_click_state & PRESSED_NOW){
        section->split_slider_mouse_drag_offset = input->mouse.p.comp[slider_comp] - (a.p.comp[slider_comp] + a.s.comp[slider_comp] * fraction);
    }

    if(slider_click_state & PRESSED) {
        auto old_value = section->split_percentage;
        
        float fraction_under_mouse = (input->mouse.p.comp[slider_comp] + section->split_slider_mouse_drag_offset - a.p.comp[slider_comp]) / a.s.comp[slider_comp];
        section->split_percentage =  clamp<int>(fraction_under_mouse * 100, 20, 80);

        if(section->split_percentage != old_value)
            dock->unsaved_changes = true;
    }

}


void ui_dock(UI_Context ctx, UI_Dock *dock, Input_Manager *input)
{
    U(ctx);

    auto *root = &dock->sections[0];
    
    Assert(dock->sections.n > 0);
    int close_requester = -1;
    int empty_view_requester = -1;
    int tab_requester = -1;
    ui_dock_section(P(ctx), root, NULL, dock, input, &empty_view_requester, &close_requester, &tab_requester);


    // CLOSE SECTION
    if(close_requester >= 0) {
        UI_Dock_Section *parent = NULL;
        int sub_section_to_close = -1;
        // @Speed
        for(int i = 0; i < dock->sections.n; i++) {
            auto *it = &dock->sections[i];
            if(it->split == UI_DOCK_SPLIT_NONE) continue;
            for(int j = 0; j < ARRLEN(it->sub_sections); j++) {
                if(it->sub_sections[j] == close_requester) {
                    parent = it;
                    sub_section_to_close = j;
                    break;
                }
            }
            if(parent) break;
        }
        Assert(parent && sub_section_to_close >= 0);
        
        Assert(parent->split == UI_DOCK_SPLIT_VERTICAL || parent->split == UI_DOCK_SPLIT_HORIZONTAL);
        merge_split_ui_dock_section(parent, sub_section_to_close, dock, ctx.client);
    }

    // PUT EMPTY VIEW IN SECTION
    if(empty_view_requester >= 0) {
        auto *sub = &dock->sections[empty_view_requester];
        Assert(sub->split == UI_DOCK_SPLIT_NONE);
        Assert(sub->view.address.type != VIEW_EMPTY);
        View empty_view;
        Zero(empty_view);
        replace_view(&sub->view, &empty_view, ctx.client, dock);
    }
}


bool write_ui_dock_section(UI_Dock_Section *section, u32 format_version, FILE *f)
{
    write_s32(section->split, f);
    switch(section->split) { // @Jai: #complete
        case UI_DOCK_SPLIT_NONE: {
            String command = command_from_view_address(section->view.address);
            if(!write_string(command, f)) return false;
        } break;

        case UI_DOCK_SPLIT_HORIZONTAL:
        case UI_DOCK_SPLIT_VERTICAL: {
            if(!write_u32(section->split_percentage, f)) return false;
            for(int i = 0; i < 2; i++)
                if(!write_u32(section->sub_sections[i], f)) return false;
        } break;

        case UI_DOCK_SPLIT_TABBED: {
            if(!write_u32(section->visible_tab, f)) return false;
        } break;
            
        default: Assert(false); return false;
    }

    return true;
}

bool read_ui_dock_section(UI_Dock_Section *_section, u32 format_version, FILE *f, Client *client)
{
    Zero(*_section);
    s32 split;
    if(!read_s32(&split, f)) return false;
    _section->split = (UI_Dock_Split_Mode)split;

    switch(_section->split) {
        
        case UI_DOCK_SPLIT_NONE: {
            String command;
            if(!read_string(&command, f)) return false;
            if(!view_address_from_command(command, client, &_section->view.address)) {
                Zero(_section->view);
            }
        } break;
            
        case UI_DOCK_SPLIT_HORIZONTAL:
        case UI_DOCK_SPLIT_VERTICAL: {
            if(!read_u32(&_section->split_percentage, f)) return false;
            for(int i = 0; i < 2; i++)
                if(!read_u32(&_section->sub_sections[i], f)) return false;
        } break;

        case UI_DOCK_SPLIT_TABBED: {
            if(!read_u32(&_section->visible_tab, f)) return false;
        } break;
            
        default: Assert(false); return false;
    }

    return true;
}

void maybe_save_ui_dock_changes_to_disk(UI_Dock *dock, double t, Client *client)
{
    const u32 format_version = 1;
    
    if(!dock->unsaved_changes || t - dock->save_t < .5) return;

    FILE *f = open_file("ui_dock", true);
    if(!f) {
        Debug_Print("ERROR: Unable to open UI dock file!\n");
        return;
    }
    defer(close_file(f););

    bool success = true;

    if(success)
        if(!write_u32(format_version, f)) success = false;

    if(success)
        if(!write_u32(dock->sections.n, f)) success = false;
    if(success)
        for(int i = 0; i < dock->sections.n; i++) {
            if(!write_ui_dock_section(&dock->sections[i], format_version, f)) success = false;
        }

    if(!success)
    {
        Debug_Print("ERROR: Failed to write UI dock to disk!\n");
        return;
    }

    dock->save_t = t;
    dock->unsaved_changes = false;
}


bool load_ui_dock_from_disk(UI_Dock *_dock, Client *client)
{
    FILE *f = open_file("ui_dock", false);
    if(!f) {
        Debug_Print("ERROR: Unable to open UI dock file!\n");
        return false;
    }
    defer(close_file(f););

    bool success = true;

    Zero(*_dock);

    u32 format_version;        
    if(success)
        if(!read_u32(&format_version, f)) success = false;

    u32 num_sections;
    if(success)
        if(!read_u32(&num_sections, f)) success = false;

    if(success) {
        array_add_uninitialized(_dock->sections, num_sections);
        for(int i = 0; i < num_sections; i++) {
            if(!read_ui_dock_section(&_dock->sections[i], format_version, f, client)) {
                success = false;
                break;
            }
        }
    }

    if(!success) {
        clear(_dock);
        Debug_Print("ERROR: Failed to read UI dock from disk!\n");
        return false;
    }

    for(int i = 0; i < _dock->sections.n; i++) {
        auto *it = &_dock->sections[i];
        if(it->split != UI_DOCK_SPLIT_NONE) continue;
        post_view_open(&it->view, client);
    }

    return true;
}
