
// @Cleanup
int fps = 0;
int frames_this_second = 0;
int ups = 0;
int updates_this_second = 0;
u64 draw_calls_last_frame = 0;
u64 triangles_this_frame = 0;


bool update_gpu_resources(Graphics *gfx)
{
    
    GPU_Texture_Parameters default_texture_params = {0};
    default_texture_params.pixel_components = GPU_PIX_COMP_RGBA;
    default_texture_params.pixel_format     = GPU_PIX_FORMAT_RGBA;
    default_texture_params.pixel_data_type  = GPU_PIX_DATA_UNSIGNED_BYTE;
    
    // GET MAX NUM MULTISAMPLE SAMPLES //
    int max_num_samples = gpu_max_num_multisample_samples();
    int num_samples = min(tweak_int(TWEAK_MAX_MULTISAMPLE_SAMPLES), max_num_samples);
    // 

    GPU_Error_Code gpu_error;
    
    // UPDATE OR CREATE MULTISAMPLE TEXTURE //
    if(!gpu_update_or_create_multisample_texture(&gfx->multisample_texture, (u64)gfx->frame_s.w, (u64)gfx->frame_s.h, num_samples, &gpu_error)) {
        Debug_Print("Failed to create multisample texture.\n");
        Assert(false);
        return false;
    }
    //
   
    // UPDATE OR CREATE DEPTH BUFFER TEXTURE //
    if(!gpu_update_or_create_multisample_depth_buffer_texture(&gfx->depth_buffer_texture, (u64)gfx->frame_s.w, (u64)gfx->frame_s.h, num_samples, &gpu_error)) {
        Debug_Print("Failed to create depth_buffer texture.\n");
        Assert(false);
        return false;
    }
    //
    
    // UPDATE MULTISAMPLE FRAMEBUFFER //
    //NOTE: @Cleanup: We only need to attach the texture to the framebuffer once. Not every time we update the texture properties...
    gpu_update_framebuffer(gfx->framebuffer, gfx->multisample_texture, true, gfx->depth_buffer_texture);
    //

    return true;
}

void frame_begin(Window *window, bool first_frame, v2 frame_s, Graphics *gfx)
{
    v2 old_frame_s = gfx->frame_s;
    gfx->frame_s = frame_s;

    if(first_frame ||
       (u64)old_frame_s.w != (u64)gfx->frame_s.w || // @Robustness: Maybe frame w and h should be floats.
       (u64)old_frame_s.h != (u64)gfx->frame_s.h)
    {
        bool gpu_resources_update_result = update_gpu_resources(gfx);
        Assert(gpu_resources_update_result);
    }

    reset(&gfx->default_vertex_buffer);
    reset(&gfx->world_render_buffer);

    // BIND TEXTURES //
    Assert(ARRLEN(gfx->bound_textures) >= TEX_NONE_OR_NUM);
    for(int t = 0; t < TEX_NONE_OR_NUM; t++)
    {
        if(gfx->num_bound_textures <= t || gfx->bound_textures[t] != (Texture_ID)t)
        {
            gpu_bind_texture(gfx->textures.ids[t], t);
            switch(t) {
                
                case 0: gpu_set_uniform_int(gfx->fragment_shader.texture_1_uniform, t); break;
                case 1: gpu_set_uniform_int(gfx->fragment_shader.texture_2_uniform, t); break;
                case 2: gpu_set_uniform_int(gfx->fragment_shader.texture_3_uniform, t); break;
                case 3: gpu_set_uniform_int(gfx->fragment_shader.texture_4_uniform, t); break;

                default: Assert(false); break;
            }

            gfx->bound_textures[t] = (Texture_ID)t;
            gfx->num_bound_textures = max(t+1, gfx->num_bound_textures);
        }
    }
    // 


    gfx->z_for_2d = 1;
    eat_z_for_2d(gfx); // So we don't draw the first thing at 1, which is the clear value.
    

    #if DEBUG
    gfx->debug.num_draw_calls = 0;
    #endif
}

void frame_end(Window *window, Graphics *gfx)
{
    // BLIT MULTISAMPLE TO DEFAULT FRAMEBUFFER //
    auto w = gfx->frame_s.w;
    auto h = gfx->frame_s.h;

    const int default_framebuffer = 0;
    
    gpu_blit_framebuffer(gfx->framebuffer,    0, 0, w, h,
                         default_framebuffer, 0, 0, w, h);
    //

    // Switch buffer set //
    gfx->buffer_set_index++;
    gfx->buffer_set_index %= ARRLEN(gfx->vertex_shader.buffer_sets);
    gpu_set_buffer_set(&gfx->vertex_shader.buffer_sets[gfx->buffer_set_index], &gfx->vertex_shader);
    //--
        
    platform_end_frame(window);
}

bool init_graphics(Window *window, Graphics *gfx)
{
    platform_init_gl_for_window(window);
    
    if(!wglMakeCurrent(window->DeviceContext, window->GLContext)) {
        Debug_Print("wglMakeCurrent error: %d\n", GetLastError());
        Assert(false);
        return false;
    }
    
    if(!gpu_init(0, 0, 0, 1)) {
        Assert(false);
        return false;
    }

#if GFX_GL    
    /// LOAD SHADERS ///
    byte *vertex_shader_code = 0;
    byte *fragment_shader_code = 0;

    char *vertex_shader_file   = "vertex_shader.glsl";
    char *fragment_shader_file = "fragment_shader.glsl";

    bool vertex_shader_loaded   = read_entire_resource(vertex_shader_file, &vertex_shader_code, ALLOC_TMP);
    bool fragment_shader_loaded = read_entire_resource(fragment_shader_file, &fragment_shader_code, ALLOC_TMP);

    if(!vertex_shader_loaded)   { Debug_Print("Failed to load vertex shader.\n");   return false; };
    if(!fragment_shader_loaded) { Debug_Print("Failed to load fragment shader.\n"); return false; }

    /// COMPILE SHADER PROGRAM ///    
    if(!gpu_compile_shaders(vertex_shader_code, fragment_shader_code, &gfx->gpu_ctx))
    {   
        Debug_Print("Failed to compile shaders.\n");
        Assert(false);
        return false;
    }
    else {
        Debug_Print("Shaders compiled successfully.\n");
    }
#endif
    
    if(!gpu_init_shaders(&gfx->vertex_shader, &gfx->fragment_shader, &gfx->gpu_ctx)) {
        Debug_Print("Failed to init shaders.\n");
        Assert(false);
        return false;
    }

    // Create multisample framebuffer
    gpu_create_framebuffers(1, &gfx->framebuffer);

    // Init World_Graphics
    gfx->world.static_opaque_vao = create_vao();
    //--
    
    return true;
}


void config_gpu_for_ui(Graphics *gfx)
{
    float a = 2.0 / gfx->frame_s.w;
    float b = 2.0 / gfx->frame_s.h;
    
    m4x4 ui_projection = {
        a, 0, 0,-1,
        0, b, 0,-1,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    gpu_set_uniform_m4x4(gfx->vertex_shader.projection_uniform, ui_projection);
    gpu_set_uniform_m4x4(gfx->vertex_shader.transform_uniform,  M_IDENTITY);
    gpu_set_uniform_bool(gfx->vertex_shader.mode_2d_uniform,    true);
    gpu_set_uniform_v4(gfx->vertex_shader.color_multiplier_uniform, { 1, 1, 1, 1 });

    gpu_set_viewport(0, 0, gfx->frame_s.w, gfx->frame_s.h);
}


void config_gpu_for_world(Graphics *gfx, Rect viewport, m4x4 projection)
{
    gpu_set_viewport(viewport.x, viewport.y, viewport.w, viewport.h);
    
    gpu_set_uniform_m4x4(gfx->vertex_shader.projection_uniform, projection);
    gpu_set_uniform_m4x4(gfx->vertex_shader.transform_uniform,  M_IDENTITY);
    gpu_set_uniform_bool(gfx->vertex_shader.mode_2d_uniform,    false);
    gpu_set_uniform_v4(gfx->vertex_shader.color_multiplier_uniform, { 1, 1, 1, 1 });
}


struct Render_Loop
{
    enum State
    {
        INITIALIZING,
        RUNNING,
        SHOULD_EXIT
    };
    
    State  state;
    Thread thread;
    Client *client;
};


void draw_panel(UI_Element *e, UI_Manager *ui, Graphics *gfx, double t)
{
    Assert(e->type == PANEL);

    _OPAQUE_UI_();
    draw_rect(e->panel.a, e->panel.color, gfx);
}

void draw_window(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
    Assert(e->type == WINDOW);
    auto *win = &e->window;
    auto a = win->current_a;
    
    {
        _TRANSLUCENT_UI_();
        const v4 shadow_c = { 0, 0, 0, 0.05 };
        draw_rect(a, shadow_c, gfx);
    }
    
    _OPAQUE_UI_();
    
    const float visible_border_w = 2;
    Rect r = shrunken(a, window_border_width-visible_border_w);
    
    draw_rect(r, win->border_color, gfx);

    r = shrunken(r, visible_border_w);

    const v4 c_white = { 1, 1, 1, 1 };

    Rect title_a = cut_top_off(&r, window_title_height);
    draw_string_in_rect_centered(get_ui_string(win->title, ui), title_a, FS_20, FONT_TITLE, c_white, gfx);
    //---------

    draw_rect(r, win->background_color, gfx);

    const v4 c_red    = { 1, 0, 0, 1 };
    if(win->resize_dir_x < 0) draw_rect(left_of(  a, window_border_width), c_red, gfx);
    if(win->resize_dir_x > 0) draw_rect(right_of( a, window_border_width), c_red, gfx);
    if(win->resize_dir_y < 0) draw_rect(bottom_of(a, window_border_width), c_red, gfx);
    if(win->resize_dir_y > 0) draw_rect(top_of(   a, window_border_width), c_red, gfx);

    // CLOSE BUTTON //
    if(win->has_close_button)
    {
        auto state = win->close_button_state;
        
        const v4 btn_c_hovered = { 0.85, 0, 0.15, 1 };
        const v4 btn_c_pressed = { 0.75, 0, 0.05, 1 };
        
        v4 btn_c = { 0.8, 0, 0.2, 1, };
        if(state & PRESSED)      btn_c = btn_c_pressed;
        else if(state & HOVERED) btn_c = btn_c_hovered;

        Rect btn_a = right_square_of(title_a);
        btn_a.h -= visible_border_w;
        draw_rect(btn_a, btn_c, gfx);
    }

    
    if(tweak_bool(TWEAK_SHOW_WINDOW_SIZES)) {
        draw_string(concat_tmp(a.w, " x ", a.h), { a.x + a.w, a.y }, FS_12, FONT_TITLE, C_WHITE, gfx, HA_RIGHT, VA_BOTTOM);
    }
}


void draw_ui_text(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
    _TRANSLUCENT_UI_();
    
    Assert(e->type == UI_TEXT);
    auto &txt = e->text;
    auto a = txt.a;

    String text = get_ui_string(txt.text, ui);
    Rect clip_rect = a;

    draw_body_text(text, txt.font_size, txt.font, a,
                   txt.color, gfx, txt.h_align,
                   &clip_rect, NULL,
                   txt.v_align);
}

// REMEMBER to do _OPAQUE_UI_() or whatever before calling this.
void _draw_button_label(String label, Rect a, Graphics *gfx)
{
    draw_string_in_rect_centered(label, a, FS_16, FONT_TITLE, {1, 1, 1, 1}, gfx);
}

// REMEMBER to do _OPAQUE_UI_() or whatever before calling this.
void _draw_button(Rect a, UI_Click_State click_state, Graphics *gfx, v4 color,
                  bool enabled = true, bool selected = false, String label = EMPTY_STRING)
{
    float z = eat_z_for_2d(gfx);
    v3 p0 = { a.x, a.y, z };
    
    float color_saturation = saturation_of(color);
    
    v4 c_idle = color;
    
    v4 c_hovered = color;
    c_hovered.rgb *= 1.1f;

    v4 c_pressed = color;
    c_pressed.rgb *= 0.6f;

    v4 c_disabled = color;
    c_disabled.rgb *= 0.9f;
    adjust_saturation(&c_disabled, 0.33f);
    
    v4 c_selected = color;
    c_selected.rgb *= 1.28f;
    //--

    v4 color_to_use;
    if(selected)                   color_to_use = c_selected;
    else if(!enabled)              color_to_use = c_disabled;
    else if(click_state & PRESSED) color_to_use = c_pressed;
    else if(click_state & HOVERED) color_to_use = c_hovered;
    else color_to_use = c_idle;

    draw_quad(p0, V3_X * a.w, V3_Y * a.h, color_to_use, gfx);
    if(label.length > 0)
        _draw_button_label(label, a, gfx);
}

void draw_button(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
    Assert(e->type == BUTTON);
    auto &btn = e->button;
    
    _OPAQUE_UI_();

    _draw_button(btn.a, btn.state, gfx, btn.color,
                btn.enabled, btn.selected, get_ui_string(btn.label, ui));
}

void draw_ui_liquid_container(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
    Assert(e->type == UI_LIQUID_CONTAINER);
    auto &ui_lc = e->liquid_container;

    _OPAQUE_UI_();

    auto *lc = &ui_lc.lc;
    auto lq_type = liquid_type_of_container(lc);

    Rect aa = ui_lc.a;
    Rect bottom_a = cut_bottom_off(&aa, 10);
    Rect meter_a  = cut_bottom_off(&aa, 14);
    Rect info_a   = aa;

    // INFO //
    {
        String type_str = (lq_type != LQ_NONE_OR_NUM) ? liquid_names[lq_type] : STRING("EMPTY");
        draw_string(type_str, center_left_of(info_a), FS_16, FONT_BODY, ui_lc.text_color, gfx, HA_LEFT, VA_CENTER);
    }
    
    // METER //
    {
        draw_rect(meter_a, { 0.1, 0.1, 0.1, 1 }, gfx);
        
        Rect inner = shrunken(meter_a, 2);
        draw_rect(inner, { 0.4, 0.4, 0.4, 1 }, gfx);

        float fill_factor = (ui_lc.capacity <= 0) ? 0 : (lc->amount / (float)ui_lc.capacity);
        draw_rect(left_of(inner, fill_factor * inner.w), liquid_color(lc->liquid), gfx);

    }

    // BOTTOM //
    {
        String str = concat_tmp(lc->amount, "/", ui_lc.capacity);
        draw_string(str, center_right_of(bottom_a), FS_10, FONT_BODY, ui_lc.text_color, gfx, HA_RIGHT, VA_CENTER);
    }
}

void draw_inventory_slot(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
    Assert(e->type == UI_INVENTORY_SLOT);
    auto &slot = e->inventory_slot;

    _OPAQUE_UI_();

    v4 background = { 0.24, 0.30, 0.38, 1 };
    
    String label = EMPTY_STRING;    
    if(slot.item_type != ITEM_NONE_OR_NUM) {
        Item_Type *type = &item_types[slot.item_type];
        label = type->name;
        Assert(label.length > 0);
        label.length = min(3, label.length);

        background = type->color;
        adjust_saturation(&background, 0.8f);
        adjust_brightness(&background, 0.8f);
    }

    
    _draw_button(slot.a, slot.click_state, gfx, background,
                 slot.enabled, slot.selected);

    auto a = slot.a;
    v3 fill_p0 = { a.x, a.y + a.h, eat_z_for_2d(gfx) };
    if(slot.fill > 0) {
        draw_quad(fill_p0, -V3_Y * a.h * slot.fill, V3_X * a.w, { 0.03, 0.8, 0.6, 1 }, gfx);
    }
    
    _draw_button_label(label, slot.a, gfx);

    if(slot.slot_flags & INV_SLOT_RESERVED) {
        draw_rect(bottom_right_of(shrunken(slot.a, 2), 10, 10), { 1, 1, 1, 1 }, gfx);
    }
}

void draw_chat(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
    Assert(e->type == UI_CHAT);
    auto *chat = &e->chat;

    _TRANSLUCENT_UI_();
    
    String text = get_ui_string(chat->text, ui);
    draw_rect(chat->a, { 1, 1, 1, 1 }, gfx);
    draw_body_text(text, FS_14, FONT_BODY, shrunken(chat->a, 2), {0.1, 0.1, 0.1, 1}, gfx);
}

void draw_textfield(UI_Element *e, UI_ID id, UI_Manager *ui, Graphics *gfx)
{
    _OPAQUE_UI_();    

    Assert(e->type == TEXTFIELD);
    auto *tf = &e->textfield;
    
    draw_rect(tf->a, {0.8, 0.8, 0.8, 1.0}, gfx);

    Rect inner_a = textfield_inner_rect(tf);
    
    Rect text_a = inner_a;
    text_a.y   += tf->scroll.value;
    
    String text = get_ui_string(tf->text, ui);
    Body_Text bt = create_textfield_body_text(text, inner_a, gfx->fonts);
    
    Rect clip_rect = tf->a;

    if(ui->active_element == id) {

        // CARET / HIGHLIGHT
        
        auto &tf_state = ui->active_textfield_state;

        // Highlight
        int highlight_cp0;
        int highlight_cp1;
        if(tf_state.caret.cp > tf_state.highlight_start.cp) {
            highlight_cp0 = tf_state.highlight_start.cp;
            highlight_cp1 = tf_state.caret.cp;
        } else {
            highlight_cp0 = tf_state.caret.cp;
            highlight_cp1 = tf_state.highlight_start.cp;
        }
        
        v2 highlight_p0 = text_a.p + position_from_codepoint_index(highlight_cp0, &bt, gfx->fonts);
        v2 highlight_p1 = text_a.p + position_from_codepoint_index(highlight_cp1, &bt, gfx->fonts);
        highlight_p0.y += text_a.h - bt.line_height;
        highlight_p1.y += text_a.h - bt.line_height;

        
        int highlight_line0 = line_from_codepoint_index(highlight_cp0, &bt);
        int highlight_line1 = line_from_codepoint_index(highlight_cp1, &bt) + 1;
        int num_highlight_lines = highlight_line1 - highlight_line0;
        for(int l = 0; l < num_highlight_lines; l++)
        {
            float start_x = text_a.x;
            float end_x   = text_a.x + text_a.w;
            
            if(l == 0)                   start_x = highlight_p0.x;
            if(l == num_highlight_lines-1) end_x = highlight_p1.x;

            // @Speed: Draw one rect for all "full" highlight lines
            // @Speed: Break this loop if we go past clip rect y1.
            
            // TODO :PushPop color
            Rect a = {start_x - 2,
                      highlight_p0.y - bt.line_height * l,
                      end_x - start_x + 2,
                      bt.line_height};
            a = rect_intersection(a, clip_rect);
            draw_rect(a, {0.10, 0.61, 0.53, 1}, gfx);
        }
        // ----------

        // Caret
        v2 caret_p;
        if(highlight_cp0 == tf_state.caret.cp) {
            caret_p = highlight_p0;
        } else {
            Assert(highlight_cp1 == tf_state.caret.cp);
            caret_p = highlight_p1;
        }
    
        Rect caret_a = {caret_p.x - 2, caret_p.y, 2, bt.line_height};
        caret_a = rect_intersection(caret_a, clip_rect);
        draw_rect(caret_a, {0, 0, 0, 1}, gfx);
        // ----------

#if DEBUG && true // DEBUG STUFF
        
#if true && DEBUG // DEBUG DISPLAY: LAST VERTICAL NAV X
        draw_rect_ps({inner_a.x + ui->active_textfield_state.last_vertical_nav_x-2,
                    inner_a.y + 4}, {4, 4}, {1, 0, 0, 0.5}, gfx);
        draw_rect_ps(inner_a.p + V2_X * (ui->active_textfield_state.last_vertical_nav_x-1), {2, inner_a.h}, {1, 0, 0, 0.5}, gfx);
#endif

#if true && DEBUG // DEBUG DISPLAY: CARET/HIGHLIGHT CODEPOINT INDEX

        { // CARET
            String cp_index_str = concat_tmp("", tf_state.caret.cp);
            float cp_index_str_w = string_width(cp_index_str, FS_12, FONT_TITLE, gfx);
        
            draw_rect_ps({inner_a.x - cp_index_str_w - 4, inner_a.y + inner_a.h - 16}, {cp_index_str_w + 4, 16}, {0, 0, 0, 1}, gfx);
            
            draw_string(cp_index_str, { inner_a.x - cp_index_str_w - 2, inner_a.y + inner_a.h - 2 }, FS_12, FONT_TITLE, {1, 1, 1, 1}, gfx);   
        }

        { // HIGHLIGHT
            String cp_index_str = concat_tmp("", tf_state.highlight_start.cp);
            float cp_index_str_w = string_width(cp_index_str, FS_12, FONT_TITLE, gfx);
        
            draw_rect_ps({inner_a.x - cp_index_str_w - 4, inner_a.y + inner_a.h - 32}, {cp_index_str_w + 4, 16}, {0.10, 0.61, 0.53, 1}, gfx);
            
            draw_string(cp_index_str, { inner_a.x - cp_index_str_w - 2, inner_a.y + inner_a.h - 2 - 16 }, FS_12, FONT_TITLE, {1, 1, 1, 1}, gfx);   
        }
        
#endif

#endif
        
    }

    
#if false && DEBUG // DEBUG DISPLAY: INNER RECT
    {
        draw_rect_ps(inner_a.p, inner_a.s, {1, 1, 1, 0.5}, gfx);
    }
#endif

    draw_body_text(text, FS_16, FONT_INPUT, text_a, {0.1, 0.1, 0.1, 1}, gfx, HA_LEFT, &clip_rect, &bt);

    if(tf->scrollbar_visible)
    {
        Rect scrollbar_a;
        Rect handle_a;
        get_textfield_scrollbar_rects(tf->a, inner_a.h,
                                      tf->scroll.value, bt.lines.n * bt.line_height,
                                      &scrollbar_a, &handle_a);

        draw_rect(scrollbar_a, {0.15, 0.8, 0.3, 1}, gfx);
        draw_rect(handle_a, {0.05, 0.15, 0.6, 1}, gfx);
    }
}

void draw_slider(UI_Element *e, Graphics *gfx)
{    
    _OPAQUE_UI_();    

    Assert(e->type == SLIDER);
    auto *slider = &e->slider;
    auto a = slider->a;

    v3 v[6] = {
        { a.x,       a.y,       0 },
        { a.x,       a.y + a.h, 0 },
        { a.x + a.w, a.y,       0 },

        { a.x + a.w, a.y,       0 },
        { a.x,       a.y + a.h, 0 },
        { a.x + a.w, a.y + a.h, 0 }
    };
        
    v2 uv[6] = {0};
    
    v4 c[6] = {
        { 0.05, 0.4, 0.5, 1 },
        { 0.05, 0.4, 0.5, 1 },
        { 0.05, 0.4, 0.5, 1 },
        
        { 0.05, 0.4, 0.5, 1 },
        { 0.05, 0.4, 0.5, 1 },
        { 0.05, 0.4, 0.5, 1 },
    };
     
    v4 d[6] = {
        { 0.30, 0.35, 0.45, 1 },
        { 0.30, 0.35, 0.45, 1 },
        { 0.30, 0.35, 0.45, 1 },
        
        { 0.30, 0.35, 0.45, 1 },
        { 0.30, 0.35, 0.45, 1 },
        { 0.30, 0.35, 0.45, 1 },
    };

    float tex[6] = {
        0, 0, 0,
        0, 0, 0
    };

    v3 normals[6] = {
        V3_Z, V3_Z, V3_Z,
        V3_Z, V3_Z, V3_Z
    };

    v4 *color = c;
    if(!slider->enabled)  color = d;

    triangles(v, uv, color, tex, normals, 6, gfx);

    v4 handle_c   = {0, 0.25, 0.45, 1};
    v4 handle_c_p = {0.1, 0.1, 0.1, 1};
    
    draw_rect(slider_handle_rect(a, slider->value), (slider->pressed) ? handle_c_p : handle_c, gfx);
}

void draw_dropdown(UI_Element *e, Graphics *gfx)
{
    _OPAQUE_UI_();    

    Assert(e->type == DROPDOWN);
    auto *dd = &e->dropdown;
    
    draw_rect(dropdown_rect(dd->box_a, dd->open), {0.8, 0.4f, 0.2f, 1.0f}, gfx);
}

// NOTE: This does not support negative values.
void draw_graph(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
    _OPAQUE_UI_();

    Assert(e->type == GRAPH);
    auto *graph = &e->graph;
    auto a = graph->a;

    // BACKGROUND //
    draw_rect(a, { 0.02, 0.12, 0.09, 1}, gfx);
    // ////////// //
    
    v4 fill_color = { 0.00, 0.59, 0.53, 0.2 };
    v4 line_color = { 0.00, 0.59, 0.53,   1 };
    v4 grid_color = { 0.00, 0.16, 0.08,   1 };
    v4 number_color = line_color;
    number_color.rgb *= 0.9f;

    String values_str = get_ui_string(graph->data, ui);
    Assert(values_str.length % sizeof(float) == 0);
    
    auto num_values = values_str.length / sizeof(float);
    float *values = (float *)values_str.data;

    Assert(graph->y_max > graph->y_min); // Make sure max > min, and avoid division by zero.
    float y_minmax_delta = graph->y_max - graph->y_min;
    
    float y_factor = a.h / y_minmax_delta;
    float y_offset = -graph->y_min * y_factor;

    const float numbers_w = 26;
    float graph_w = a.w - numbers_w;

    // NUMBERS BACKGROUND
    draw_rect(right_of(a, numbers_w), { 0, 0, 0, 0.6 }, gfx);

    // GRID
    float magnitude       = floor(log10(y_minmax_delta));
    s64 grid_ystep        = roundf(roundf(y_minmax_delta / 10.0f / (5.0f * magnitude)) * 5.0f * magnitude);
    
    if(grid_ystep > 0) {

        s64 grid_yy = 0;
        while(grid_yy <= graph->y_min)             grid_yy += grid_ystep;
        while(grid_yy - grid_ystep > graph->y_min) grid_yy -= grid_ystep;

        int line_ix = 0;
        while(grid_yy < graph->y_max)
        {
            v2 p = a.p + V2_Y * (((grid_yy - graph->y_min) / y_minmax_delta) * a.h);
            
            draw_rect_ps(p, { a.w, 1 }, grid_color, gfx);

            // @Norelease: Do K, M, G suffixes, so we fit.
            if(p.y > a.y + 22)
                draw_string(s64_to_string(grid_yy, ALLOC_TMP), { a.x + a.w - 2, p.y - 2 }, FS_12, FONT_BODY, number_color, gfx, HA_RIGHT);
            
            grid_yy += grid_ystep;
            line_ix++;
        }
    }

    const int max_grid_cols = 20;
    int num_grid_cols = num_values;
    while(num_grid_cols > max_grid_cols)
    {
        if(num_grid_cols % 2 == 0) num_grid_cols /= 2;
        else num_grid_cols = max_grid_cols;
    }

    num_grid_cols -= 1;
        
    for(int x = 1; x < num_grid_cols; x++)
    {
        draw_rect_ps(a.p + V2_X * (x * (graph_w/(num_grid_cols))), { 1, a.h }, grid_color, gfx);
    }
    // --
    
    
    float fill_z = eat_z_for_2d(gfx);
    float line_z = eat_z_for_2d(gfx);
    
    v3 origin = { a.x, a.y + y_offset, fill_z };
    float section_w = (num_values > 1) ? graph_w / (num_values-1) : graph_w;

    // GRAPH //
    for(int i = 0; i < num_values; i++)
    {
        float y1 = values[i];
        float y0;
        
        if(i == 0) {
            if(num_values > 1) continue;
            else {
                y0 = y1;
                i += 1;
            }
        }
        else y0 = values[i-1];

        v3 p0 = origin + V3_X * (i-1) * section_w;
        v3 p1 = p0 + V3_X * section_w;
        v3 p2 = p0 + V3_Y * (y0 * y_factor);
        v3 p3 = p1 + V3_Y * (y1 * y_factor);

        // Fill down to the bottom, even if 0 is higher up.
        p0.y = a.y;
        p1.y = a.y;
        
        v3 p[6] = {
            p0, p1, p2,
            p2, p1, p3
        };

        draw_polygon<ARRLEN(p)>(p, fill_color, gfx);

        v3 line_p0 = p2;
        v3 line_p1 = p3;
        line_p0.z = line_z;
        line_p1.z = line_z;
        draw_line(line_p0, line_p1, V3_Z, 2, line_color, gfx);


        // Last value sign
        if(i == num_values-1)
        {
            Rect aa = { { line_p1.x, line_p1.y - 18 }, { numbers_w, 18 } };
            
            draw_rect(aa, { 0, 0, 0, 0.8 }, gfx);
            draw_string(s64_to_string((s64)y1, ALLOC_TMP), center_bottom_of(aa), FS_18, FONT_BODY, line_color, gfx, HA_CENTER, VA_BOTTOM);
            draw_line(line_p1, line_p1 + V3_X * numbers_w, V3_Z, 2, line_color, gfx);
        }
    }
    // ///// //
    
}


// NOTE: tp is tile position, which is (min x, min y, z) of the tile
// NOTE: selected_item can be null.
void draw_tile_hover_indicator(v3 tp, Item *item_to_place, Quat placement_q, double world_t, Room *room, Input_Manager *input, Client *client, Graphics *gfx)
{
    if(tp.x >= 0 && tp.x <= room_size_x - 1 && 
       tp.y >= 0 && tp.y <= room_size_y - 1)
    {
        if(item_to_place)
        {
            Quat q = placement_q;
            
            // @Norelease: Should show where the player must stand when putting the item down, so we know why it is not possible if the player position is blocked, but the item is not.
            
            bool can_be_placed = true;
            
            v4 shadow_color = { 0.12, 0.12, 0.12, 1 };
            if(!can_place_item_entity_at_tp(item_to_place, tp, q, world_t, room)) {
                shadow_color = { 0.4,   0.1,  0.1, 1 };
                can_be_placed = false;
            }
            
            Entity preview_entity = create_preview_item_entity(item_to_place, tp, q, world_t);
            draw_entity(&preview_entity, world_t, room, client, gfx, false, /*cannot_be_placed = */!can_be_placed);

            Assert(preview_entity.type == ENTITY_ITEM);

            _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
            AABB bbox = entity_aabb(&preview_entity, world_t, room);
            draw_quad(bbox.p + V3_Z * 0.001f, {bbox.s.x, 0, 0}, {0, bbox.s.y, 0}, shadow_color, gfx);
            
        }
        else {
            _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
            draw_quad(tp + V3(-1, -1, 0.001f), V3_X * 2.0f, V3_Y * 2.0f, { 1, 0, 0, 1 }, gfx);
        }
    }
}


void draw_ui_chess_board(UI_Element *e, UI_Manager *ui, Graphics *gfx, Client *client)
{
    _OPAQUE_UI_();
    
    Assert(e->type == UI_CHESS_BOARD);
    auto *chess = &e->chess_board;
    
    String board_str = get_ui_string(chess->board, ui);
    Assert(board_str.length == sizeof(Chess_Board));

    Chess_Board *board = (Chess_Board *)board_str.data;

    Chess_Move *queued_move = (chess->queued_move.from != chess->queued_move.to) ? &chess->queued_move : NULL;
    draw_chess_board(board, chess->a, gfx, current_user_id(client), chess->selected_square_ix, queued_move);
    
}


m4x4 draw_world_view(UI_Element *e, Room *room, double t, Input_Manager *input, Client *client, Graphics *gfx, User *user = NULL)
{
    _OPAQUE_UI_();

    Assert(e->type == WORLD_VIEW);
    auto *wv = &e->world_view;
    
    double world_t = world_time_for_room(room, t);
    
    Player_State player_state_after_queue = {0}; // IMPORTANT: Only valid if player != NULL.
    auto *player = find_current_player_entity(client);
    if(player) {
        player_state_after_queue = player_state_after_completed_action_queue(player, world_t, room);
    }

    // BACKGROUND //
    draw_rect(wv->a, { 0.17, 0.15, 0.14, 1 }, gfx);

    // PROJECTION MATRIX //
    m4x4 projection = world_projection_matrix(wv->a, -0.1 + gfx->z_for_2d);

    { Scoped_Push(gfx->draw_mode, DRAW_3D);

        Item *item_to_place = NULL;
        
        if(in_array(input->keys, VKEY_CONTROL)) // @Volatile: Set this keybinding somewhere, we have this if in both the draw code and the "control" code. But it's probably @Temporary anyway. @Norelease
        {
            if(player && player_state_after_queue.held_item.type != ITEM_NONE_OR_NUM) {    
                // PUT DOWN ITEM //
                item_to_place = &player_state_after_queue.held_item;
            }
        }
        else {
            // PLACE FROM INVENTORY //
            item_to_place = (user) ? get_selected_inventory_item(user) : NULL;
        }

        // DRAW WORLD //
        draw_world(room, world_t, projection, client, gfx, wv->hovered_entity, wv->mouse_ray);

        // SURFACE HIGHLIGHT //
        if(wv->surface_is_hovered) {
            auto surf = wv->hovered_surface;

            v4 color = C_YELLOW;
            switch(surf.type) {
                case SURF_TYPE_MACHINE_INPUT:  color = C_RED; break;
                case SURF_TYPE_MACHINE_OUTPUT: color = C_GREEN; break;
            }
            
            _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
            draw_quad(surf.p + V3_Z * 0.002f, V3_X * surf.s.x, V3_Y * surf.s.y, color, gfx);
        }

        // HOVERED TILE, ITEM PREVIEW //
        if(wv->hovered_entity == NO_ENTITY || wv->surface_is_hovered)
        {
            v3 hovered_tp;
            if(item_to_place) hovered_tp = room->placement_tp;
            else              hovered_tp = tp_from_index(wv->hovered_tile_ix);

            draw_tile_hover_indicator(hovered_tp, item_to_place, room->placement_q, world_t, room, input, client, gfx);
        }
    }

    
    
    // EAT Z //
    gfx->z_for_2d -= 0.2;

    return projection;
}


DWORD render_loop(void *loop_)
{
    Render_Loop *loop = (Render_Loop *)loop_;

    Client *client = loop->client;
    UI_Manager    *ui;    
    Input_Manager *input;    
    Window        *main_window;
    
    Graphics gfx = {0};

    // START INITIALIZATION //
    lock_mutex(client->mutex);
    {
        Assert(loop->state == Render_Loop::INITIALIZING);
        
        ui          = &client->ui;
        input       =  &client->input;
        main_window = &client->main_window;
        
        gfx.fonts  = &client->fonts;
        gfx.assets = &client->assets;
        
        bool graphics_init_result = init_graphics(main_window, &gfx);
        Assert(graphics_init_result);

        init_fonts(gfx.fonts, &gfx);
        init_assets_for_drawing(&client->assets, &gfx);

        // CREATE FONT GLYPH TEXTURES ON GPU //
        for(int f = 0; f < NUM_FONTS; f++){
            GPU_Error_Code error_code = 0;
            create_gpu_texture_for_sprite_map(&gfx.glyph_maps[f], &gfx, &error_code);
            Assert(error_code == 0);
        }
        //

        // INITIALIZATION DONE //        
        loop->state = Render_Loop::RUNNING;
    }
    unlock_mutex(client->mutex);
    
    bool first_frame = true;
    u64 last_second = platform_get_time();

    Array<Rect, ALLOC_MALLOC> dirty_rects = {0};
    defer(clear(&dirty_rects););
    
    while(true)
    {        
        // @Cleanup?
        bool world_view_exists = false;
        UI_World_View world_view;
        m4x4 world_projection;
    
        lock_mutex(client->mutex);
        {
            triangles_this_frame = 0;
            double t       = platform_get_time();
            
            if(loop->state == Render_Loop::SHOULD_EXIT) {
                unlock_mutex(client->mutex);
                break;
            }
                
#if DEBUG
            draw_calls_last_frame = gfx.debug.num_draw_calls;
#endif
            
            frame_begin(main_window, first_frame, client->main_window_a.s, &gfx);
            
            u64 second = platform_get_time();


            // Find dirty rects //
            if(first_frame || true /* @Norelease */) {
                Rect frame_a = { 0, 0, gfx.frame_s.w, gfx.frame_s.h }; // TODO: Do this when window size changes
                array_add(dirty_rects, frame_a);
            }
            
            if(dirty_rects.n > 0)
            {
                Scoped_Push(gfx.draw_mode, DRAW_2D);
                
                // Draw UI //
                const v4 background_color = { 0.3, 0.36, 0.42, 1 };
                draw_rect(rect(0, 0, gfx.frame_s.w, gfx.frame_s.h), background_color, &gfx);
            
                for(int i = ui->elements_in_depth_order.n-1; i >= 0; i--)
                {
                    UI_ID id      = ui->elements_in_depth_order[i];
                    UI_Element *e = find_ui_element(id, ui);
                    Assert(e);

                    switch(e->type) {
                        case PANEL:    draw_panel(e, ui, &gfx, t);   break;
                        case WINDOW:   draw_window(e, ui, &gfx);  break;
                        case UI_TEXT:  draw_ui_text(e, ui, &gfx); break;
                        case BUTTON:   draw_button(e, ui, &gfx);  break;
                        case TEXTFIELD: draw_textfield(e, id, ui, &gfx); break;
                        case SLIDER:   draw_slider(e, &gfx);      break;
                        case DROPDOWN: draw_dropdown(e, &gfx);    break;

                        case GRAPH: draw_graph(e, ui, &gfx); break;

                        case UI_LIQUID_CONTAINER: draw_ui_liquid_container(e, ui, &gfx); break;
                        case UI_INVENTORY_SLOT: draw_inventory_slot(e, ui, &gfx);  break;
                        case UI_CHAT:           draw_chat(e, ui, &gfx);  break;
                            
                        case UI_CHESS_BOARD:    draw_ui_chess_board(e, ui, &gfx, client);  break;

                        case WORLD_VIEW: {
                            auto *room = &client->room;

                            if(world_view_exists) {
                                Assert(false);
                                break;
                            }

                            m4x4 projection = draw_world_view(e, room, t, input, client, &gfx, current_user(client));

                            auto &graphics = gfx; // @Hack @Stupid @Cleanup
                            
                            if(!client->connections.room.connected &&
                               !client->connections.room_connect_requested)
                            {
                                auto *gfx = &graphics;
                                _TRANSLUCENT_UI_();
                                draw_rect(e->world_view.a, { 0, 0, 0, 0.5f }, gfx);
                            }
                            
                            world_view_exists = true;
                            world_view = e->world_view;
                            world_projection = projection;
                                
                        } break;

#if DEBUG
                        case UI_PROFILER: {
                            draw_ui_profiler(e, PROFILER, input, &gfx);
                        } break;
#endif

                        default: Assert(false); break;
                    }
                }
            }

            if(second != last_second) {
                fps = frames_this_second;
                frames_this_second = 0;
            }
            
            frames_this_second++;
            
            last_second = second;
            first_frame = false;
        }
        unlock_mutex(client->mutex);

        gpu_set_vsync_enabled(tweak_bool(TWEAK_VSYNC));
            
        auto *default_buffer_set = current_default_buffer_set(&gfx);

        maybe_push_vao_to_gpu(&gfx.world.static_opaque_vao, &gfx.universal_vertex_buffer, &gfx);
        
        for(int i = 0; i < ARRLEN(gfx.glyph_maps); i++)
        {
            update_sprite_map_texture_if_needed(&gfx.glyph_maps[i], &gfx);
        }

        gpu_disable_scissor();
        
        for(int i = 0; i < dirty_rects.n; i++)
        {
            Rect dirty_rect = dirty_rects[i];

            gpu_set_scissor(dirty_rect.x, gfx.frame_s.h - dirty_rect.y - dirty_rect.h, dirty_rect.w, dirty_rect.h);
            {   
                gpu_set_depth_testing_enabled(true);
                gpu_set_depth_mask(true);
                gpu_clear_depth_buffer();
                
                // DEFAULT VERTEX BUFFER (@Temporary?)
                config_gpu_for_ui(&gfx); 
                {
                    gpu_set_depth_mask(false);
                    draw_vertex_buffer(&gfx.default_vertex_buffer, true, default_buffer_set);
                }

                // OPAQUE UI //
                config_gpu_for_ui(&gfx); 
                {
                    gpu_set_depth_mask(true);
                    draw_render_object_buffer(&gfx.ui_render_buffer.opaque, false, &gfx);
                }

                // WORLD //
                if(world_view_exists) {
                    config_gpu_for_world(&gfx, world_view.a, world_projection);
                    {
                        // Opaque
                        gpu_set_depth_mask(true);
                        draw_render_object_buffer(&gfx.world_render_buffer.opaque, false, &gfx);

                        // Static Opaque
                        gpu_set_uniform_m4x4(gfx.vertex_shader.transform_uniform, M_IDENTITY);
                        draw_vao(&gfx.world.static_opaque_vao, &gfx);

                        // Translucent
                        gpu_set_depth_mask(false);
                        draw_render_object_buffer(&gfx.world_render_buffer.translucent, true, &gfx);
                    }
                }

                // TRANSLUCENT UI //
                config_gpu_for_ui(&gfx);
                {
                    gpu_set_depth_mask(false);
                    draw_render_object_buffer(&gfx.ui_render_buffer.translucent, true, &gfx);              
                }
                
            }
            gpu_disable_scissor();


        }
        

        // Reset render buffers //
        reset_vertex_buffer(&gfx.default_vertex_buffer);
        reset_render_object_buffer(&gfx.ui_render_buffer.opaque);
        reset_render_object_buffer(&gfx.world_render_buffer.opaque);
        reset_render_object_buffer(&gfx.world_render_buffer.translucent);
        reset_render_object_buffer(&gfx.ui_render_buffer.translucent);
        // ------------ //
        
        Assert(current_vertex_buffer(&gfx) == &gfx.default_vertex_buffer);
        
        // Make sure we've flushed all vertex buffers //
        Assert(gfx.default_vertex_buffer.n == 0);
        Assert(gfx.world_render_buffer.opaque.vertices.n == 0);
        Assert(gfx.world_render_buffer.opaque.objects.n == 0);
        Assert(gfx.world_render_buffer.translucent.vertices.n == 0);
        Assert(gfx.world_render_buffer.translucent.objects.n == 0);
        //
        
        frame_end(main_window, &gfx);

        // Reset dirty rects //
        dirty_rects.n = 0;
    }

    return 0;
}

// NOTE: Assumes you've zeroed *_loop
bool start_render_loop(Render_Loop *_loop, Client *client)
{
    _loop->state = Render_Loop::INITIALIZING;
    _loop->client = client;

    // Start thread
    if(!create_thread(&render_loop, _loop, &_loop->thread)) {
        return false;
    } 

    // Wait for the loop to initialize itself.
    while(true) {
        lock_mutex(client->mutex);
        defer(unlock_mutex(client->mutex););

        if(_loop->state != Render_Loop::INITIALIZING) {
            Assert(_loop->state == Render_Loop::RUNNING);
            break;
        }
    }

    return true;
}

void stop_render_loop(Render_Loop *loop, Client *client)
{
    lock_mutex(client->mutex);
    Assert(loop->state == Render_Loop::RUNNING);
    loop->state = Render_Loop::SHOULD_EXIT;
    unlock_mutex(client->mutex);

    join_thread(loop->thread);
}
