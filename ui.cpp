

// IMPORTANT: Callers assume strings pushed after each other will end up after each other in UI_Manager.string_data. (Except if there is a reset inbetween the calls).
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

        case WORLD_VIEW:
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

        case WORLD_VIEW:
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



UI_Click_State evaluate_click_state(UI_Click_State state, bool hovered, Input_Manager *input, bool disabled = false)
{
    auto &mouse = input->mouse;
    
    state &= ~CLICKED_AT_ALL;
    state &= ~CLICKED_DISABLED;
    state &= ~CLICKED_ENABLED;
    state &= ~PRESSED_NOW;
    
    if(hovered) {
        state |= HOVERED;
        
        if(mouse.buttons_down & MB_PRIMARY) {
            if(!(state & PRESSED)) state |= PRESSED_NOW;
            state |= PRESSED;
        }

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



float scrollbar_handle_height(float scrollbar_h, float content_h, float view_h)
{
    if(content_h  >= 0.0001f) // Avoid division by zero
        return max(72, scrollbar_h * (view_h / content_h));

    return scrollbar_h;
}

Rect scrollbar_handle_rect(Rect scrollbar_a, float content_h, float view_h, float scroll)
{
    float h = scrollbar_handle_height(scrollbar_a.h, content_h, view_h);
    Rect a = top_of(scrollbar_a, h);
    
    float space      = scrollbar_a.h - h;
    float max_scroll = max(0, content_h - view_h);
    
    if(max_scroll > 0.0001f) // Avoid division by zero
        a.y += (scroll / max_scroll) * space;

    return a;
}

void update_scrollbar(UI_Scrollbar *scroll, bool scrollbar_hovered,
                      Rect scrollbar_a, float content_h, float view_h,
                      Input_Manager *input, bool disabled = false)
{
    auto *mouse = &input->mouse;

    Rect handle_a = scrollbar_handle_rect(scrollbar_a, content_h, view_h, scroll->value);

    bool handle_hovered = (scrollbar_hovered && point_inside_rect(mouse->p, handle_a));
    scroll->handle_click_state = evaluate_click_state(scroll->handle_click_state, handle_hovered, input, disabled);

    float scroll_max = (content_h - view_h);

    if(scroll->handle_click_state & PRESSED) {
        
        if(scroll->handle_click_state & PRESSED_NOW) {
            scroll->handle_grab_rel_p = mouse->p.y - handle_a.y;
        }

        float space_h  = scrollbar_a.h - handle_a.h;

        float handle_rel_y = mouse->p.y - scroll->handle_grab_rel_p - scrollbar_a.y;

        if(space_h > 0.0001f) // Avoid division by zero
            scroll->value = (handle_rel_y/space_h) * scroll_max;
        else
            scroll->value = 0;
    }
    
    scroll->value = max(0, min(scroll_max, scroll->value));
}


void ui_text(String text, UI_Context ctx)
{
    U(ctx);
    
    UI_Element *e = find_or_create_ui_element(ctx.get_id(), UI_TEXT, ctx.manager);

    auto *txt = &e->text;
    txt->a    = area(ctx.layout);
    txt->text = push_ui_string(text, ctx.manager);
}


UI_Click_State button(UI_Context ctx, String label = EMPTY_STRING, bool disabled = false, bool selected = false)
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
    
    btn.state = evaluate_click_state(btn.state, e == hovered_element, input, btn.disabled);
}


Rect slider_handle_rect(Rect slider_a, float value)
{
    Rect handle_a = left_square_of(slider_a);
    handle_a.x += (slider_a.w - handle_a.w) * value;

    return handle_a;
}

inline
bool is_allowed_input(u32 cp, bool single_line_mode /*= false*/)
{
    if(cp == '\n') {
        return !single_line_mode;
    }

    if(cp < 32) return false; // @Temporary: TODO: @NoRelease: We need to disallow all unicode codepoints we don't have glyphs for.
 
    return true;
}


const float textfield_scrollbar_w = 10;

inline
Rect textfield_scrollbar_rect(Rect textfield_a)
{
    return right_of(textfield_a, textfield_scrollbar_w);
}

void get_textfield_scrollbar_rects(Rect textfield_a, float textfield_inner_h, float scroll,
                                   float text_h, Rect *_scrollbar_a, Rect *_handle_a)
{
    Rect scrollbar_a = textfield_scrollbar_rect(textfield_a);

    Rect handle_a = scrollbar_handle_rect(scrollbar_a, text_h, textfield_inner_h, scroll);
    
    *_handle_a    = handle_a;
    *_scrollbar_a = scrollbar_a;
}

Rect textfield_inner_rect(UI_Textfield *tf)
{
    return shrunken(tf->a, 10, textfield_scrollbar_w, 8, 8);
}

Body_Text create_textfield_body_text(String text, Rect inner_a, Font *fonts)
{
    return create_body_text(text, inner_a, FS_16, FONT_INPUT, fonts);
}


String textfield_tmp(String text, Input_Manager *input, UI_Context ctx, bool *_text_did_change)
{
    // IMPORTANT: Don't use the carets before we've clamped them to the text length. (We do that further down in this proc) -EH, 2020-11-13
    
    U(ctx);

    const bool multiline = true; // @Temporary

    UI_Manager *ui = ctx.manager;

    auto id = ctx.get_id();
    UI_Element *e = find_or_create_ui_element(id, TEXTFIELD, ui);
    auto *tf = &e->textfield;
    tf->a    = area(ctx.layout);


    // TEXT //
    *_text_did_change = false;

    if(ui->active_element != id) {
        tf->text = push_ui_string(text, ui);
        return text;
    }

    // ////////////////////////////////////// //
    // THE TEXTFIELD IS ACTIVE IF WE GET HERE //
    // ////////////////////////////////////// //
    
    Rect inner_a = textfield_inner_rect(tf);

    // RESET LAST NAV DIR ON RESIZE //
    auto *tf_state = &ui->active_textfield_state;
    if(fabs(inner_a.w - tf_state->last_resize_w) > 0.001f) {
        tf_state->last_nav_dir  = NO_DIRECTION;
        tf_state->last_resize_w = inner_a.w;
    }
    // /////////////////////////// //
    
    auto *caret           = &ui->active_textfield_state.caret;
    auto *highlight_start = &ui->active_textfield_state.highlight_start;

    u32 text_cp_length = count_codepoints(text);
    
    // CLAMP CARETS //
    if(caret->cp > text_cp_length) {
        caret->cp   = text_cp_length;
    }
        
    if(highlight_start->cp > text_cp_length) {
        highlight_start->cp   = text_cp_length;
    }
    // /////////// //


    if(input->text.n == 0) {
        tf->text = push_ui_string(text, ui);
    }
    else
    {
        // @BadName because we set this to say that we want to erase things after caret.
        strlength initial_byte_caret = codepoint_start(ui->active_textfield_state.caret.cp, text.data, text.data + text.length) - text.data; // @Speed
        
        // INSERT TEXT //
        u8 *input_at  = input->text.e;
        u8 *input_end = input_at + input->text.n;

        // Erase highlight?
        bool do_erase_highlight = false;
        if(highlight_start->cp != caret->cp)
        {
            while(input_at < input_end) {
                u8 *cp_start = input_at;
                u32 cp = eat_codepoint(&input_at);

                if(cp == '\b') {
                    do_erase_highlight = true;
                    // We don't set input_at back here, because we want to consume one backspace to erase the highlight.
                    break;
                }

                if(!is_allowed_input(cp, !multiline)) continue;

                do_erase_highlight = true;
                input_at = cp_start;
                break;
            }
        }

        // Erase highlight
        if(do_erase_highlight) {
            if(caret->cp > highlight_start->cp) {
                *caret = *highlight_start;
            } else {
                // @Hack...
                initial_byte_caret = codepoint_start(highlight_start->cp, text.data, text.data + text.length) - text.data; // @Speed
                *highlight_start = *caret;
            }

            *_text_did_change = true;
        }

        // Check if we should erase at insertion point. See comment in Input_Manager for an explanation of why we can do it like this.
        u8 *pre_end = codepoint_start(caret->cp, text.data, text.data + text.length); // @Speed
        while(input_at < input_end)
        {
            u8 *cp_start = input_at;
            u32 cp = eat_codepoint(&input_at);
            
            if(cp != '\b') {
                input_at = cp_start;
                break;
            }

            if(pre_end > text.data) {
                u8 *erased_cp_end = pre_end;
                pre_end = find_codepoint_backwards(pre_end);

                caret->cp   -= 1;
                *highlight_start = *caret;

                *_text_did_change = true;
            }
            else {
                Assert(pre_end == text.data);
            }
        }

        // Push text before insertion point
        tf->text = push_ui_string({text.data, pre_end - text.data}, ui);

        // Push the new input
        int cp_ix = 0;

        while(input_at < input_end)
        {
            u8 *cp_start = input_at;
            u32 cp = eat_codepoint(&input_at);

            Assert(cp != '\b'); // See comment in Input_Manager for an explanation of why this always should be true.
            
            if(!is_allowed_input(cp, !multiline)) continue;

            strlength byte_length = input_at - cp_start;
            push_ui_string({cp_start, byte_length}, ui);
                
            tf->text.length += byte_length;
            
            caret->cp   += 1;
            *highlight_start = *caret;

            *_text_did_change = true;
        }

        // Push text after insertion point
        if(initial_byte_caret < text.length) {
            auto end_length = (strlength)(text.length - initial_byte_caret);
            push_ui_string({text.data + initial_byte_caret, end_length}, ui);
            tf->text.length += end_length;
        }

#if DEBUG // TODO Make a DEBUG_SLOW
        {
            auto length = count_codepoints(get_ui_string(tf->text, ui));
            Assert(ui->active_textfield_state.caret.cp >= 0);
            Assert(ui->active_textfield_state.caret.cp <= length);

            Assert(ui->active_textfield_state.highlight_start.cp >= 0);
            Assert(ui->active_textfield_state.highlight_start.cp <= length);
        }
#endif
    }

    // //// //

    ui->active_textfield_state.text_did_change = *_text_did_change;

    if(!(*_text_did_change)) {        
        return text;
    }
    
    return get_ui_string(tf->text, ui);
}




// NOTE: Only one direction can be passed -- dir is not used as a bitmask.
// NOTE: If the caret moved, the index of the line of the caret is returned. Otherwise -1.   
int textfield_navigate(Direction dir, bool shift_is_down, Body_Text *bt, UI_Textfield_State *tf_state, Font *fonts)
{        
    String &text = bt->text;
    auto *caret = &tf_state->caret;
    
    int cp_delta = 0;

    bool did_go = false;
    int line = -1;

    switch(dir) {
        case UP:
        case DOWN: {
            
            // NAVIGATE VERTICALLY //

            // THE CURRENT LINE //
            // Line and col
            int current_col;
            int current_line_index = line_from_codepoint_index(caret->cp, bt, &current_col);
            auto *current_line = bt->lines.e + current_line_index;
            // //////////////// //

            // CAN GO? //
            int new_line_index = current_line_index;
            if(dir == UP) {
                new_line_index -= 1;
                if(new_line_index < 0) break; // Can't go.
            } else {
                Assert(dir == DOWN);
                new_line_index += 1;
                if(new_line_index >= bt->lines.n) break; // Can't go.
            }
            // /////// //

            // THE LINE TO GO TO //
            Assert(new_line_index >= 0 && new_line_index < bt->lines.n);
            auto *new_line = bt->lines.e + new_line_index;

            int new_line_start_cp_index   = new_line->start_cp;
            strlength new_line_start_byte = new_line->start_byte;
            
            // Find end byte
            strlength new_line_end_byte;
            if(new_line_index < bt->lines.n-1) {
                auto *line_after_new = bt->lines.e + new_line_index + 1;
                new_line_end_byte = line_after_new->start_byte;
            } else {
                new_line_end_byte = bt->text.length;
            }
            
            // Start and end of new line
            u8 *new_line_start = bt->text.data + new_line_start_byte;
            u8 *new_line_end   = bt->text.data + new_line_end_byte;

            // //////////////// //

            
            // X TO USE TO FIND COL ON NEW LINE //
            float x;
            if(tf_state->last_nav_dir == UP ||
               tf_state->last_nav_dir == DOWN) {
                x = tf_state->last_vertical_nav_x;
            }
            else {
                u8 *current_line_start = text.data + current_line->start_byte;
                // @Robustness: We allow x_from_codepoint_index to search to the end of the text.
                //              We should find our codepoint index before reaching the line's end byte,
                //              but we don't make sure that we do...
                x = x_from_codepoint_index(current_col, current_line_start, text.data + text.length, bt, fonts);
                tf_state->last_vertical_nav_x = x;
            }
            // //////////////////////////////// //
            

            // FIND NEW CARET //
            caret->cp = new_line_start_cp_index + codepoint_index_from_x(x, new_line_start, new_line_end, bt, fonts, true);
            // /////////// //

            line = new_line_index;
            did_go = true;
            
        } break;

        case LEFT:  {
            // NAVIGATE LEFT //
            if(codepoint_start(caret->cp, text.data, text.data + text.length) > text.data)
            {    
                caret->cp--;

                line = line_from_codepoint_index(caret->cp, bt);
                did_go = true;
            }
        } break;

        case RIGHT: {
            // NAVIGATE RIGHT //
            u8 *end = text.data + text.length;

            if(codepoint_start(caret->cp, text.data, text.data + text.length) < end)
            {    
                caret->cp++;

                line = line_from_codepoint_index(caret->cp, bt);
                did_go = true;
            }
        } break;

        default: Assert(false); return -1;
    }

    // Navigating without holding SHIFT removes highlight
    if(!shift_is_down) {
        tf_state->highlight_start = *caret;
    }

    if(did_go)
        tf_state->last_nav_dir = dir;

    return line;
}

void reset_textfield_state(UI_Textfield_State *tf_state)
{
    Zero(*tf_state);
}

// NOTE: If the line is already fully visible, the scroll won't change.
void textfield_scroll_to_line(int line_index, float inner_h, float line_h, int num_lines, UI_Scrollbar *scroll)
{
    Assert(line_index >= 0 && line_index < num_lines);
    
    float line_y0 = line_h * line_index;
    float line_y1 = line_y0 + line_h;

    if(scroll->value > line_y0) {
        scroll->value = line_y0;
    } else if(scroll->value + inner_h < line_y1) {
        scroll->value = line_y1 - inner_h;
    }

    scroll->value = clamp(scroll->value, 0.0f, max(0, (line_h * num_lines) - inner_h));
}

void textfield_scroll_to_codepoint_index(u64 cp_index, float inner_h, Body_Text *bt, UI_Scrollbar *scroll)
{
    int line = line_from_codepoint_index(cp_index, bt);
    textfield_scroll_to_line(line, inner_h, bt->line_height, bt->lines.n, scroll);
}

void update_textfield(UI_Element *e, UI_ID id, Input_Manager *input, UI_Element *hovered_element, UI_Manager *ui, Font *fonts, bool became_active, double t, bool *_use_i_beam_cursor)
{
    Assert(e->type == TEXTFIELD);
    auto *tf = &e->textfield;
    auto *mouse = &input->mouse;

    Rect inner_a = textfield_inner_rect(tf);
    String text  = get_ui_string(tf->text, ui);
    Body_Text bt = create_textfield_body_text(text, inner_a, fonts);
    
    float text_h = bt.lines.n * bt.line_height;
    tf->scrollbar_visible = (text_h > inner_a.h);
    
    bool area_hovered = (e == hovered_element);
    if(tf->scrollbar_visible)
    {
        Rect scrollbar_a;
        Rect handle_a;
        get_textfield_scrollbar_rects(tf->a, inner_a.h, tf->scroll.value, text_h, &scrollbar_a, &handle_a);

        bool scrollbar_hovered = false;
        if(point_inside_rect(mouse->p, scrollbar_a)) {
            area_hovered = false;
            scrollbar_hovered = true;
        }
        
        update_scrollbar(&tf->scroll, scrollbar_hovered, scrollbar_a, text_h, inner_a.h, input);
    }
    else {
        tf->scroll.value = 0;
    }

    
    tf->click_state = evaluate_click_state(tf->click_state, area_hovered, input, tf->disabled);

    
    *_use_i_beam_cursor = (area_hovered && !(tf->scroll.handle_click_state & PRESSED));

        
    if(tf->disabled) return;
    
    if(ui->active_element != id) return;

    // //////////////////////////////////////////////// //
    // IF WE REACH THIS POINT, THE TEXTFIELD IS ACTIVE. //
    // //////////////////////////////////////////////// //

    auto *tf_state = &ui->active_textfield_state;
    auto *caret           = &tf_state->caret;
    auto *highlight_start = &tf_state->highlight_start;
    
    Rect text_a = inner_a;
    text_a.h  = text_h;
    text_a.y -= tf->scroll.value;
    

    if(became_active) {
        ui->active_textfield_state.last_resize_w = text_a.w;
    }

    bool shift_is_down = false;
    bool ctrl_is_down  = false;
    
    for(int i = 0; i < input->keys.n; i++) {
        if(input->keys[i] == VKEY_SHIFT)   { shift_is_down = true; continue; }
        if(input->keys[i] == VKEY_CONTROL) { ctrl_is_down = true; continue; }
    }
    
    // SELECT ALL //
    if(ctrl_is_down && in_array(input->keys_down, VKEY_a)) {
        
        highlight_start->cp = 0;

        caret->cp   = bt.num_codepoints;
    }
    // ////////// //

    bool do_scroll_to_caret = tf_state->text_did_change;
    
    if(tf->click_state & PRESSED) {
        
        Text_Location mouse_text_location = text_location_from_position(mouse->p, &bt, text_a.p, fonts);

        caret->cp = mouse_text_location.cp_index;

        // Scroll to caret?
        if(t - tf_state->last_scroll_to_caret_by_mouse_t >= tweak_float(TWEAK_SCROLL_TO_CARET_REPEAT_INTERVAL))
        {
            do_scroll_to_caret = true;
            tf_state->last_scroll_to_caret_by_mouse_t = t;
        }
        
        if(tf->click_state & PRESSED_NOW && !shift_is_down) {
            highlight_start->cp   = mouse_text_location.cp_index;
        };

    }

    // NAVIGATE //
    {
        for(int i = 0; i < input->key_hits.n; i++)
        {
            Direction nav_dir;
            
            virtual_key key = input->key_hits[i];
            switch(key) {
                case VKEY_LEFT:  nav_dir = LEFT;  break;
                case VKEY_RIGHT: nav_dir = RIGHT; break;
                case VKEY_UP:    nav_dir = UP;    break;
                case VKEY_DOWN:  nav_dir = DOWN;  break;
                default: continue;
            }

            int line = textfield_navigate(nav_dir, shift_is_down, &bt, &ui->active_textfield_state, fonts);
            if(line != -1) do_scroll_to_caret = true;
        }
    }
    // ///////// //

    if(do_scroll_to_caret) {
        textfield_scroll_to_codepoint_index(tf_state->caret.cp, inner_a.h, &bt, &tf->scroll);
    }
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

    dd->box_click_state = evaluate_click_state(dd->box_click_state, box_hovered, input);
    
    if(dd->box_click_state & CLICKED_ENABLED) dd->open = !dd->open;
    else if(dd->open && !(dd->box_click_state & HOVERED)) {
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

Rect begin_window(UI_ID *_id, UI_Context ctx, String title = EMPTY_STRING, bool use_default_padding = true,
                  bool use_custom_border_color = false, v4 custom_border_color = V4_ZERO)
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
    win->border_color = (use_custom_border_color) ? custom_border_color : tweak_v4(TWEAK_WINDOW_BORDER_COLOR);

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

void end_window(UI_ID id, UI_Manager *ui, UI_Click_State *_close_button_state = NULL)
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
        win->close_button_state = evaluate_click_state(win->close_button_state, close_button_hovered, input);
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

// NOTE: (@Temporary): Returns index of clicked tile. U64_MAX if none.
u64 world_view(UI_Context ctx)
{
    U(ctx);
    
    UI_Element *e = find_or_create_ui_element(ctx.get_id(), WORLD_VIEW, ctx.manager);
    
    auto *view = &e->world_view;
    view->a = area(ctx.layout);

    view->camera.projection = world_projection_matrix(view->a);
    view->camera.projection_inverse = inverse_of(view->camera.projection);

    return view->clicked_tile_ix;
}

void update_world_view(UI_Element *e, Input_Manager *input, UI_Element *hovered_element)
{    
    Assert(e->type == WORLD_VIEW);
    auto *view = &e->world_view;
    auto *mouse = &input->mouse;

    view->clicked_tile_ix = U64_MAX;
    view->click_state = evaluate_click_state(view->click_state, e == hovered_element, input);

    if(e == hovered_element)
    {
        Assert(point_inside_rect(mouse->p, view->a));
        int hovered_tile_x = (mouse->p.x - view->a.x) / (view->a.w / room_size_x);
        int hovered_tile_y = (mouse->p.y - view->a.y) / (view->a.h / room_size_y);
        u64 hovered_tile_ix = hovered_tile_y * room_size_x + hovered_tile_x;
        
        if(view->click_state & PRESSED_NOW) {
            view->pressed_tile_ix = hovered_tile_ix;
        }
        else if(view->click_state & CLICKED_ENABLED) {
            if(view->pressed_tile_ix == hovered_tile_ix) {
                view->clicked_tile_ix = view->pressed_tile_ix;
            }
        }
    }

    if(!(view->click_state & PRESSED))
        view->pressed_tile_ix = U64_MAX;

    view->mouse_p = input->mouse.p;
    view->mouse_ray = screen_point_to_ray(input->mouse.p, view->a, view->camera.projection_inverse);
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

void end_ui_build(UI_Manager *ui, Input_Manager *input, Font *fonts, double t, Cursor_Icon *_cursor)
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
            case WINDOW:     mouse_over = point_inside_rect(mouse.p, e->window.current_a); break;
            case BUTTON:     mouse_over = point_inside_rect(mouse.p, e->button.a);         break;
            case TEXTFIELD:  mouse_over = point_inside_rect(mouse.p, e->textfield.a);      break;
            case SLIDER:     mouse_over = point_inside_rect(mouse.p, e->slider.a);         break;
            case DROPDOWN:   mouse_over = point_inside_rect(mouse.p, dropdown_rect(e->dropdown.box_a, e->dropdown.open));   break;
            case WORLD_VIEW: mouse_over = point_inside_rect(mouse.p, e->button.a);         break;

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


    UI_Element *element_that_became_active = NULL;
    
    if(input->mouse.buttons_down & MB_PRIMARY)
    {
        if(hovered_element &&
           hovered_element->type == TEXTFIELD) // @Cleanup
        {
            Assert(hovered_element->type == TEXTFIELD);

            // Activate if mouse is not on scrollbar:
            if(ui->active_element != hovered_element_id &&
               !point_inside_rect(mouse.p, textfield_scrollbar_rect(hovered_element->textfield.a))) {
                
                // ACTIVATE ELEMENT //
                ui->active_element = hovered_element_id;
                reset_textfield_state(&ui->active_textfield_state);
            
                element_that_became_active = hovered_element;
            }
        }
        else {
            ui->active_element = NO_UI_ELEMENT;
        }   
    }

    bool use_i_beam_cursor = false;

    // UPDATE ELEMENTS //
    for(s64 i = 0; i < ui->elements.n; i++)
    {
        UI_Element *e = &ui->elements[i];
        
        switch(e->type) {
            case WINDOW:    update_window(e, ui->element_ids[i], input, hovered_element, hovered_element_id, ui); break;
            case BUTTON:    update_button(e, input, hovered_element);    break;
            case TEXTFIELD: {
                bool became_active = (element_that_became_active == e);
                bool area_hovered;
                update_textfield(e, ui->element_ids[i], input, hovered_element, ui, fonts, became_active, t, &area_hovered);
                if(area_hovered)
                    use_i_beam_cursor = true;
            } break;
            case SLIDER:     update_slider(e, input, hovered_element);     break;
            case DROPDOWN:   update_dropdown(e, input, hovered_element);   break;
            case WORLD_VIEW: update_world_view(e, input, hovered_element); break;

            case UI_TEXT:
                break;
                
            default: Assert(false); break;
        }
    }
    

    if(use_i_beam_cursor)
        *_cursor = CURSOR_ICON_I_BEAM;
    else
        *_cursor = CURSOR_ICON_DEFAULT;
}
