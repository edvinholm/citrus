

inline
UI_String push_ui_string(String string, UI_Manager *ui)
{
    UI_String str = {0};
    str.offset = ui->string_data.n;
    str.length = string.length;
    array_add(ui->string_data, string.data, string.length);

    return str;
}

//IMPORTANT: This is a temporary string that will not be valid after begin_ui_build, or after push_ui_string.
inline
String get_ui_string(UI_String string, UI_Manager *ui)
{
    Assert(string.offset >= 0);
    Assert(string.length >= 0);
    Assert(string.offset + string.length <= ui->string_data.n);

    String str = {0};
    str.data   = ui->string_data.e + string.offset;
    str.length = string.length;
    return str;
}


void init_ui_manager(UI_Manager *manager)
{
}


inline
void assert_state_valid(UI_ID_Manager *manager)
{
}

inline
void assert_state_valid(UI_Manager *ui)
{
    auto &id_manager = ui->id_manager;
    
    assert_state_valid(&id_manager);

    Assert(ui->element_ids.n == ui->elements.n);
    Assert(ui->element_ids.n == ui->element_alives.n);
}

u8 ui_path_hash(UI_Path *path, u64 length)
{    
    Assert(length < MAX_ID_PATH_LENGTH);
    u8 hash = 0;
    for(u64 l = 0; l < length; l++)
    {
        UI_Location loc = path->e[l];

        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF;
    }

    return hash;
}

UI_ID find_or_create_ui_id_for_path(UI_Path *path, u64 length, UI_ID_Manager *manager)
{
    u8 hash = ui_path_hash(path, length);

    Assert(hash < ARRLEN(manager->path_buckets));
    auto *bucket = manager->path_buckets + hash;

    Assert(bucket->path_lengths.n == bucket->paths.n);
    Assert(bucket->path_lengths.n == bucket->ids.n);
    for(u64 p = 0; p < bucket->path_lengths.n; p++)
    {
        if(bucket->path_lengths[p] != length) continue;

        bool found = true;
            
        auto &path_in_bucket = bucket->paths[p];
        for(u64 l = 0; l < length; l++) {
            
            Assert(path_in_bucket.e[l] != 0);
            
            if(path_in_bucket.e[l] != path->e[l]) {
                found = false;
                break;
            }
            
        }

        if(!found) continue;

        return bucket->ids[p];
    }


    // NOT FOUND, SO CREATE //

    UI_ID id = ++manager->last_id;
    Assert(id != 0);
    array_add(bucket->path_lengths,  length);
    array_add(bucket->paths,        *path);
    array_add(bucket->ids,           id);

    return id;
    
}

void push_ui_location(u32 place, u32 count, UI_Manager *ui)
{
    Assert(place != 0);
    
    assert_state_valid(ui);


    // Make location
    UI_Location loc = ((u64)place << 32) | ((u64)count << 0);

    // Push location
    Assert(ui->current_path_length < MAX_ID_PATH_LENGTH);
    ui->current_path.e[ui->current_path_length++] = loc;

#if DEBUG && false
    Debug_Print("\nCurrent path:\n");
    Debug_Print("[Hash: %02X]\n", ui_path_hash(&ui->current_path, ui->current_path_length));
    for(u64 i = 0; i < ui->current_path_length; i++) {
        if(i > 0) Debug_Print("      >> ");
        Debug_Print("%I64X", ui->current_path.e[i]);
    }
    Debug_Print("\n");
    for(u64 i = 0; i < ui->current_path_length; i++) {
        if(i > 0) Debug_Print(" >> ");

        UI_Location l = ui->current_path.e[i];
        
        u16 place = (l >> 32) & 0xFFFFFFFF;
        u16 count = (l >>  0) & 0xFFFFFFFF;
        
        Debug_Print("%08X|%08X", place, count);
    }
    Debug_Print("\n\n");
#endif
}

void pop_ui_location(UI_Manager *ui)
{
    Assert(ui->current_path_length > 0);
    ui->current_path_length--;
}


// @Cleanup
void init_ui_element(UI_Element_Type type, UI_Element *_e)
{
    Zero(*_e);
    _e->type  = type;

    switch(_e->type) {
        case WINDOW:
        case BUTTON:
        case TEXTFIELD:
        case SLIDER:
        case DROPDOWN:
        case UI_TEXT:
            break;
            
        default: Assert(false); break;
    }
}

void clear_ui_element(UI_Element *e)
{
    switch(e->type) {
        case DROPDOWN: clear(&e->dropdown); break;

        case WINDOW:
        case BUTTON:
        case TEXTFIELD:
        case SLIDER:
        case UI_TEXT:
            break;

        default: Assert(false); break;
    }
}


// NOTE: This DOES NOT set the element as alive.
// NOTE: This DOES NOT add the element to ui->elements_in_depth_order.
UI_Element *find_ui_element(UI_ID id, UI_Manager *ui, s64 *_index = NULL)
{
    assert_state_valid(ui);

    for(s64 i = 0; i < ui->element_ids.n; i++)
    {
        if(ui->element_ids.e[i] == id)
        {
            UI_Element *e = ui->elements.e + i;

            if(_index) *_index = i;
            return e;
        }
    }

    return NULL;
}

// NOTE: This sets the element as alive.
// NOTE: This adds the element to ui->elements_in_depth_order.
UI_Element *find_or_create_ui_element(UI_ID id, UI_Element_Type type, UI_Manager *ui, bool *_was_created = NULL)
{
    assert_state_valid(ui);

    if(_was_created) *_was_created = false;

    s64 index;
    UI_Element *e = find_ui_element(id, ui, &index);
    if(e) {   
        Assert(e->type == type);
        ui->element_alives[index] = true;
    }
    else {
        // NO ELEMENT FOUND -- SO CREATE ONE.
        if(_was_created) *_was_created = true;
    
        UI_Element new_element;
        init_ui_element(type, &new_element);
        array_add(ui->element_alives, true);
        array_add(ui->element_ids,      id);
        e = array_add(ui->elements, new_element);
    }
    
    Assert(e);

    array_add(ui->elements_in_depth_order, id);
    return e;
}





struct UI_Context
{
    UI_Manager     *manager;
    Layout_Manager *layout;
    Client         *client;
    
    enum State
    {
        UNPACKED  = 0,
        PACKED    = 1,
        DELIVERED = 2
    };
    
    UI_Context()
    {
        Zero(*this);
    }

    UI_Context(const UI_Context &ctx)
    {
        memcpy(this, &ctx, sizeof(*this));
        
        if(returning_from_pack) {
            returning_from_pack = false;
            return;
        }

        if(this->state == PACKED) {
            this->id_given = false;
        
            this->state = DELIVERED;
            return;
        }
        else {
            printf("ERROR: UI_Context must be PACKED on copy.\n");
            Assert(false);
        }
    }


    UI_ID get_id()
    {
        if(id_given) {
            Debug_Print("ERROR: A UI_Context for a specific path can only give an ID once!");
            Assert(false);
        }
        
        Assert(state == UNPACKED);
        id_given = true;
        UI_ID id = find_or_create_ui_id_for_path(&manager->current_path, manager->current_path_length, &manager->id_manager);

#if DEBUG
        if(in_array(manager->id_manager.used_ids_this_build, id)) {
            Debug_Print("ERROR: Same ID used multiple times in the same build.\n");
            Assert(false);
        }

        array_add(manager->id_manager.used_ids_this_build, id);
#endif
        
        return id;
    }

    
    UI_Context pack(u32 place, u32 count)
    {
        Assert(!returning_from_pack);
        
        UI_Context ctx = UI_Context();
    
        if(this->state != UNPACKED) {
            printf("ERROR: UI_Context must be unpacked on pack().\n");
            Assert(false);
        }
    
        memcpy(&ctx, this, sizeof(*this));

        push_ui_location(place, count, this->manager);
    
        // NOTE: Unfortunately, when returning the context, the copy constructor for UI_Context will be called
        //       To prevent that constructor from thinking that this is a parameter-pass-to-proc,
        //       we set a special state (PACKING) here, and the constructor will then set it to PACKED, and not DELIVERED.
        ctx.state = PACKED;

        ctx.id_given = false;

        ctx.returning_from_pack = true;
        return ctx;
    }

    // IMPORTANT: THIS IS NOT A FULL UNPACK. Use the UNPACK() macro!
    void set_state_to_unpacked() {
        Assert(!returning_from_pack);
        
        state = UNPACKED;
    }
    
    State get_state() { return this->state; }


    ~UI_Context()
    {
        if(returning_from_pack) return;
        
        if(state != UNPACKED) {
            Debug_Print("All UI_Contexts must be UNPACKED before being destroyed! A context is being deconstructed with state set to DELIVERED, which should never happen.\n");
            bool context_unpacked = false;
            Assert(context_unpacked); // All delivered contexts should be unpacked.
        }
    }


private:    
    State state;
    bool returning_from_pack;
    bool id_given;
};



UI_Button_State evaluate_button_state(UI_Button_State state, bool hovered, Input_Manager *input, bool disabled = false)
{
    auto &mouse = input->mouse;
    
    state &= ~CLICKED_AT_ALL;
    state &= ~CLICKED_DISABLED;
    state &= ~CLICKED_ENABLED;
    if(hovered) {
        state |= HOVERED;
        
        if(mouse.buttons_down & MB_PRIMARY)
            state |= PRESSED;

        if(state & PRESSED && (mouse.buttons_up & MB_PRIMARY)) {
            if(disabled) {
                state |= CLICKED_DISABLED;
                state |= CLICKED_AT_ALL;
            } else {
                state |= CLICKED_ENABLED;
                state |= CLICKED_AT_ALL;
            }
        }
    }
    else {
        state &= ~HOVERED;
    }
    
    if(!(mouse.buttons & MB_PRIMARY)) {
        state &= ~PRESSED;
    }

    return state;
}


void ui_text(String text, UI_Context ctx)
{
    U(ctx);
    
    UI_Element *e = find_or_create_ui_element(ctx.get_id(), UI_TEXT, ctx.manager);

    auto *txt = &e->text;
    txt->a    = area(ctx.layout);
    txt->text = push_ui_string(text, ctx.manager);
}


UI_Button_State button(UI_Context ctx, String label = EMPTY_STRING, bool disabled = false, bool selected = false)
{    
    U(ctx);
    
    UI_Element *e = find_or_create_ui_element(ctx.get_id(), BUTTON, ctx.manager);
    
    auto *btn = &e->button;
    btn->a = area(ctx.layout);
    btn->label = push_ui_string(label, ctx.manager);
    btn->disabled = disabled;
    btn->selected = selected;

    return btn->state;
}

void update_button(UI_Element *e, Input_Manager *input, UI_Element *hovered_element)
{
    Assert(e->type == BUTTON);
    auto &btn   = e->button;
    
    btn.state = evaluate_button_state(btn.state, e == hovered_element, input, btn.disabled);
}


Rect slider_handle_rect(Rect slider_a, float value)
{
    Rect handle_a = left_square_of(slider_a);
    handle_a.x += (slider_a.w - handle_a.w) * value;

    return handle_a;
}


String textfield(String text, Input_Manager *input, UI_Context ctx)
{
    U(ctx);

    UI_Element *e = find_or_create_ui_element(ctx.get_id(), TEXTFIELD, ctx.manager);
    auto *tf = &e->textfield;
    tf->a    = area(ctx.layout);
    tf->text = push_ui_string(text, ctx.manager);
    
    return text;
}

float slider(float value, UI_Context ctx, bool disabled = false)
{    
    U(ctx);

    bool was_created;
    UI_Element *e = find_or_create_ui_element(ctx.get_id(), SLIDER, ctx.manager, &was_created);
    
    auto *slider = &e->slider;
    slider->a = area(ctx.layout);
    slider->disabled = disabled;
    
    if(!slider->pressed) slider->value = value;

    return slider->value;
}

void update_slider(UI_Element *e, Input_Manager *input, UI_Element *hovered_element)
{
    auto &mouse = input->mouse;
    
    Assert(e->type == SLIDER);
    auto *slider = &e->slider;
    auto &a = slider->a;
    
    Rect handle_a = slider_handle_rect(slider->a, slider->value);

        
    if(slider->disabled || !(mouse.buttons & MB_PRIMARY))
        slider->pressed = false;

    if(!slider->disabled)
    {
        if(e == hovered_element && mouse.buttons_down & MB_PRIMARY) {
            if(point_inside_rect(mouse.p, handle_a)) {
                slider->pressed = true;
            }
        }
    }
    
    if(slider->pressed) {
        slider->value = min(1.0f, max(0.0f, (mouse.p.x - a.x - handle_a.w/2.0f) / (a.w - handle_a.w)));
    }
}


float dropdown_list_height()
{
    return 256;
}

Rect dropdown_rect(Rect box_a, bool open)
{
    Rect a = box_a;
    if(open)
        a.h += dropdown_list_height();

    return a;
}

void dropdown(UI_Context ctx)
{    
    U(ctx);
    
    UI_Element *e = find_or_create_ui_element(ctx.get_id(), DROPDOWN, ctx.manager);
    
    auto *dd = &e->dropdown;
    dd->box_a = area(ctx.layout);

    //TODO @Incomplete Options
}

void update_dropdown(UI_Element *e, Input_Manager *input, UI_Element *hovered_element)
{
    auto &mouse = input->mouse;
    
    Assert(e->type == DROPDOWN);
    auto *dd = &e->dropdown;
    auto &box_a = dd->box_a;

    bool box_hovered = false;
    if(e == hovered_element) {
        box_hovered = point_inside_rect(mouse.p, dd->box_a);
    }

    dd->box_button_state = evaluate_button_state(dd->box_button_state, box_hovered, input);
    
    if(dd->box_button_state & CLICKED_ENABLED) dd->open = !dd->open;
    else if(dd->open && !(dd->box_button_state & HOVERED)) {
        if(mouse.buttons_down & MB_PRIMARY) dd->open = false;
    }
}




const float window_default_padding =  4;
const float window_border_width    =  8;
const float window_title_height    = 24;

void update_window_move_and_resize(UI_Element *e, Client *client)
{
    Assert(e->type == WINDOW);
    auto *win = &e->window;

    if(!win->moving && win->resize_dir_x == 0 && win->resize_dir_y == 0)
        return;
    
    // NOTE: (@Speed?) Processing input for every window. (This is to get the absolute latest mouse position..)
    //       IMPORTANT: (@Robustness) When we are doing this, the mouse can be at a different location during window resize/move
    //                                than during the update phase.
    platform_process_input(&client->main_window);
    
    auto &mouse = client->input.mouse;

    v2 min_size = { 250, 250 };

    // RESIZE IF RESIZING //
    if(win->resize_dir_x == 1) {
        win->current_a.w = round(mouse.p.x - win->current_a.x + window_border_width/2.0f);
        win->current_a.w = max(min_size.w, win->current_a.w);
    }
    else if(win->resize_dir_x == -1) {
        float delta = (win->current_a.x + window_border_width/2.0f) - mouse.p.x;
        delta = max(min_size.w - win->current_a.w, delta);
        win->current_a.w += delta;
        win->current_a.x -= delta;
        
        win->current_a.w = round(win->current_a.w);
        win->current_a.x = round(win->current_a.x);
    }
    
    if(win->resize_dir_y == 1) {
        win->current_a.h = round(mouse.p.y - win->current_a.y + window_border_width/2.0f);
        win->current_a.h = max(min_size.h, win->current_a.h);
    }
    else if(win->resize_dir_y == -1) {
        float delta = (win->current_a.y + window_border_width/2.0f) - mouse.p.y;
        delta = max(min_size.h - win->current_a.h, delta);
        win->current_a.h += delta;
        win->current_a.y -= delta;
        
        win->current_a.h = round(win->current_a.h);
        win->current_a.y = round(win->current_a.y);
    }

    // //////////////// //

    // MOVE IF MOVING //
    if(win->moving)
    {
        win->current_a.p = mouse.p - win->mouse_offset_on_move_start;
    }
    // ////////////// //
}

Rect begin_window(UI_Context ctx, UI_ID *_id, String title = EMPTY_STRING, bool use_default_padding = true)
{
    U(ctx);

    UI_ID id = ctx.get_id();
    auto *ui = ctx.manager;

    bool was_created;
    UI_Element *e = find_or_create_ui_element(id, WINDOW, ui, &was_created);

    if(was_created) {
        array_add(ui->window_stack, id);
    }

    auto *win = &e->window;
    win->initial_a = area(ctx.layout);
    if(!win->was_resized_or_moved)
        win->current_a = win->initial_a;

    win->title = push_ui_string(title, ui);

    // NOTE: We do this when we begin the window, instead of in the UI update phase.
    //       Because we don't want the children of the window to lag behind. This
    //       would happen otherwise, because the children's positions would be based
    //       on the window's last frame's rect.
    update_window_move_and_resize(e, ctx.client);
    

    *_id = id;

    float to_shrink = window_border_width;
    if(use_default_padding)
        to_shrink += window_default_padding;
    
    return shrunken(win->current_a,
                    to_shrink,  // Left
                    to_shrink,  // Right
                    to_shrink + window_title_height, // Top
                    to_shrink); // Bottom
}

void end_window(UI_ID id, UI_Manager *ui, UI_Button_State *_close_button_state = NULL)
{
    UI_Element *e = find_ui_element(id, ui);
    Assert(e);
    Assert(e->type == WINDOW);
    auto *win = &e->window;

    if(_close_button_state) {
        *_close_button_state = win->close_button_state;
        win->has_close_button = true;
    } else {
        win->close_button_state = IDLE;
        win->has_close_button = false;
    }
    

    s64 old_depth_index;
    if(!in_array(ui->elements_in_depth_order, id, &old_depth_index)) {
        Assert(false);
    }

    win->num_children_above = (ui->elements_in_depth_order.n-1) - old_depth_index;

    array_add(ui->elements_in_depth_order, id);
    array_ordered_remove(ui->elements_in_depth_order, old_depth_index);
}

void update_window(UI_Element *e, UI_ID id, Input_Manager *input, UI_Element *hovered_element, UI_ID hovered_element_id, UI_Manager *ui)
{
    auto &mouse = input->mouse;
    
    Assert(e->type == WINDOW);
    UI_Window *win = &e->window;

    // AREAS //
    Rect left_border   = left_of(  win->current_a, window_border_width);
    Rect right_border  = right_of( win->current_a, window_border_width);
    Rect top_border    = top_of(   win->current_a, window_border_width);
    Rect bottom_border = bottom_of(win->current_a, window_border_width);
        
    Rect inner_a_title_included = shrunken(win->current_a, window_border_width);
    Rect title_a = top_of(inner_a_title_included, window_title_height);
    //

    // CLOSE BUTTON //
    if(win->has_close_button) {
        bool close_button_hovered = false;
        if(e == hovered_element) {
            Rect close_button_a = right_square_of(title_a);
            close_button_hovered = point_inside_rect(mouse.p, close_button_a);
        }
        win->close_button_state = evaluate_button_state(win->close_button_state, close_button_hovered, input);
    }
    //

    if(!(mouse.buttons & MB_PRIMARY)) {
        // PRESS END //
        win->pressed = false;
        win->resize_dir_x = 0;
        win->resize_dir_y = 0;
        win->moving = false;
    }

    bool move_to_top = false;
    
    if(!win->pressed && (mouse.buttons_down & MB_PRIMARY))
    {   
        if(e == hovered_element) {
            // PRESS START //    
            win->pressed = true;

            if(point_inside_rect(mouse.p, left_border))  win->resize_dir_x -= 1;
            if(point_inside_rect(mouse.p, right_border)) win->resize_dir_x += 1;
                
            if(point_inside_rect(mouse.p, top_border))    win->resize_dir_y -= 1;
            if(point_inside_rect(mouse.p, bottom_border)) win->resize_dir_y += 1;
            
            if(point_inside_rect(mouse.p, title_a)) {
                // Move Start //
                win->moving = true;
                win->mouse_offset_on_move_start = mouse.p - win->current_a.p;
            }
            
            move_to_top = true;
        }
        else if(point_inside_rect(mouse.p, win->current_a)) {
            // Check if a child is hovered.
            s64 depth_index;
            if(!in_array(ui->elements_in_depth_order, id, &depth_index)) { Assert(false); return; }

            for(int c = depth_index; c >= depth_index - win->num_children_above; c--)
            {
                if(hovered_element_id == ui->elements_in_depth_order[c]) {
                    move_to_top = true;
                    break;
                }
            }
        }
        // /////// //
    }    
    
    // MOVE TO TOP //
    if(move_to_top) {
        s64 index_in_stack;
        if(in_array(ui->window_stack, id, &index_in_stack)) {
            array_ordered_remove(ui->window_stack, index_in_stack);
            array_add(ui->window_stack, id);
        } else {
            Assert(false);
        }
    }

    
    // Remember that we moved or resized.
    if(win->moving || win->resize_dir_x != 0 || win->resize_dir_y != 0)
        win->was_resized_or_moved = true;
    
}






void begin_ui_build(UI_Manager *ui)
{    
    Assert(ui->elements.n == ui->elements_in_depth_order.n);
    ui->elements_in_depth_order.n = 0;

    ui->string_data.n = 0;

#if DEBUG
    ui->id_manager.used_ids_this_build.n = 0;
#endif
}

void end_ui_build(UI_Manager *ui, Input_Manager *input)
{
    Array<u8, ALLOC_TMP> temp = {0};


    
    Assert(ui->current_path_length == 0);
    
    s64 num_removed = 0;

    Assert(ui->element_alives.n == ui->elements.n);
    Assert(ui->element_alives.n == ui->element_ids.n);
        
    // Remove elements that were not built (a.k.a. not alive) this time.
    for(s64 i = 0; i < ui->element_alives.n; i++)
    {
        // Alive -- reset it.
        if(ui->element_alives[i]) {
            ui->element_alives[i] = false;
            continue;
        }

        // Dead -- remove it.
        clear_ui_element(ui->elements.e + i);
        
        s64 window_index;
        if(in_array(ui->window_stack, ui->element_ids[i], &window_index)) {
            array_ordered_remove(ui->window_stack, window_index);
        }
                    
        array_ordered_remove(ui->elements,       i);
        array_ordered_remove(ui->element_ids,    i);
        array_ordered_remove(ui->element_alives, i);
        i--;

        num_removed++;
    }

    Assert(ui->elements_in_depth_order.n == ui->elements.n);


    // SORT WINDOWS //
    for(s64 i = 0; i < ui->elements_in_depth_order.n; i++)
    {
        UI_ID id = ui->elements_in_depth_order[i];
        
        s64 index_in_stack;
        if(!in_array(ui->window_stack, id, &index_in_stack)) continue; // Not a window.

        // Found a window.

        // If someone is below us (> index) in depth, but over us (> index) in stack, we want to move ourself down in depth(> index)

        s64 depth_index_to_move_to = i;
        
        for(s64 j = i+1; j < ui->elements_in_depth_order.n; j++)
        {
            UI_ID other_id = ui->elements_in_depth_order[j];
                
            s64 other_index_in_stack;
            if(!in_array(ui->window_stack, other_id, &other_index_in_stack)) continue; // Not a window.

            // Found a window below us.

            Assert(other_index_in_stack != index_in_stack);
            if(other_index_in_stack < index_in_stack) continue; // It should be below us, so skip.

            depth_index_to_move_to = j+1;
        }

        if(depth_index_to_move_to != i) {
            Assert(depth_index_to_move_to > i);
            
            UI_Element *e = find_ui_element(id, ui);
            Assert(e);
            Assert(e->type == WINDOW);
            auto *win = &e->window;
            Assert(i >= win->num_children_above);

            s64 old_n = ui->elements_in_depth_order.n;

            s64 num_ids_to_move = win->num_children_above + 1;
            s64 first_id_index = i - win->num_children_above;
            
            array_set(temp, (u8 *)(ui->elements_in_depth_order.e + first_id_index), sizeof(UI_ID) * num_ids_to_move);
            array_insert(ui->elements_in_depth_order, (UI_ID *)temp.e, depth_index_to_move_to, num_ids_to_move);
            array_ordered_remove(ui->elements_in_depth_order, first_id_index, num_ids_to_move);

            Assert(old_n = ui->elements_in_depth_order.n);
#if DEBUG
            for (s64 k = 0; k < ui->elements_in_depth_order.n; k++) {
                Assert(find_ui_element(ui->elements_in_depth_order[k], ui));
            }
#endif

            i -= win->num_children_above;
        }
    }


    
    auto &mouse = input->mouse;

    // FIND HOVERED ELEMENT //
    UI_Element *hovered_element = NULL;
    UI_ID hovered_element_id = 0;
    for(s64 i = 0; i < ui->elements_in_depth_order.n; i++)
    {
        UI_ID id = ui->elements_in_depth_order[i];
        UI_Element *e = find_ui_element(id, ui); // TODO @Speed
        Assert(e);

        bool mouse_over = false;
        
        switch(e->type) {
            case WINDOW:    mouse_over = point_inside_rect(mouse.p, e->window.current_a); break;
            case BUTTON:    mouse_over = point_inside_rect(mouse.p, e->button.a);         break;
            case TEXTFIELD: mouse_over = point_inside_rect(mouse.p, e->textfield.a);      break;
            case SLIDER:    mouse_over = point_inside_rect(mouse.p, e->slider.a);         break;
            case DROPDOWN:  mouse_over = point_inside_rect(mouse.p, dropdown_rect(e->dropdown.box_a, e->dropdown.open));   break;

            case UI_TEXT:
                break;
                
            default: Assert(false); break;
        }

        if(mouse_over) {
            hovered_element = e;
            hovered_element_id = id;
            break;
        }
    }

    // UPDATE ELEMENTS //
    for(s64 i = 0; i < ui->elements.n; i++)
    {
        UI_Element *e = &ui->elements[i];

        switch(e->type) {
            case WINDOW: update_window(e, ui->element_ids[i], input, hovered_element, hovered_element_id, ui); break;
            case BUTTON: update_button(e, input, hovered_element); break;
            case TEXTFIELD: break;
            case SLIDER: update_slider(e, input, hovered_element); break;
            case DROPDOWN: update_dropdown(e, input, hovered_element); break;

            case UI_TEXT:
                break;
                
            default: Assert(false); break;
        }
    }
}
