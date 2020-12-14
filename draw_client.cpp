
// @Temporary
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
    platform_begin_frame(window);
    gpu_frame_init();

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
    //@Temporary: Move to GPU layer @Cleanup @Cleanup @Cleanup
#if 0
    glBlitNamedFramebuffer(gfx->multisample_framebuffer, 0, 0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
#else
    GLint old_framebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_framebuffer);
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gfx->framebuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    auto err = glGetError();
    Assert(err == 0);

    glBindFramebuffer(GL_FRAMEBUFFER, old_framebuffer);
#endif
    //

    // Switch buffer set //
    gfx->buffer_set_index++;
    gfx->buffer_set_index %= ARRLEN(gfx->vertex_shader.buffer_sets);
    gpu_set_buffer_set(gfx->buffer_set_index, &gfx->vertex_shader);
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


    return true;
}


void config_gpu_for_ui(Graphics *gfx)
{
    m4x4 ui_projection = make_m4x4(
        2.0/gfx->frame_s.w,  0, 0, -1,
        0, -2.0/gfx->frame_s.h, 0,  1,
        0,  0, 1,  0,
        0,  0, 0,  1);

    gpu_set_uniform_m4x4(gfx->vertex_shader.projection_uniform, ui_projection);
    gpu_set_uniform_m4x4(gfx->vertex_shader.transform_uniform,  M_IDENTITY);

    gpu_set_viewport(0, 0, gfx->frame_s.w, gfx->frame_s.h);
}


void config_gpu_for_world(Graphics *gfx, Rect viewport, m4x4 projection)
{
    gpu_set_viewport(viewport.x, gfx->frame_s.h - viewport.y - viewport.h, viewport.w, viewport.h);
    
    gpu_set_uniform_m4x4(gfx->vertex_shader.projection_uniform, projection);
    gpu_set_uniform_m4x4(gfx->vertex_shader.transform_uniform,  M_IDENTITY);
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

    draw_rect(r, c_white, gfx);

    const v4 c_red    = { 1, 0, 0, 1 };
    if(win->resize_dir_x < 0) draw_rect(left_of(  a, window_border_width), c_red, gfx);
    if(win->resize_dir_x > 0) draw_rect(right_of( a, window_border_width), c_red, gfx);
    if(win->resize_dir_y < 0) draw_rect(top_of(   a, window_border_width), c_red, gfx);
    if(win->resize_dir_y > 0) draw_rect(bottom_of(a, window_border_width), c_red, gfx);

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
}


void draw_ui_text(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
    _TRANSLUCENT_UI_();
    
    Assert(e->type == UI_TEXT);
    auto &txt = e->text;
    auto a = txt.a;

    // @Temporary @Cleanup @Robustness: :PushPop color
    String text = get_ui_string(txt.text, ui);
    Rect clip_rect = a;
    draw_body_text(text, FS_14, FONT_BODY, a, {0.1, 0.1, 0.1, 1}, gfx, false, &clip_rect);
}

void draw_button(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
    _OPAQUE_UI_();    

    Assert(e->type == BUTTON);
    auto &btn = e->button;
    auto a = btn.a;

    float z = eat_z_for_2d(gfx);
    
    v3 v[6] = {
        { a.x,       a.y,       z },
        { a.x,       a.y + a.h, z },
        { a.x + a.w, a.y,       z },

        { a.x + a.w, a.y,       z },
        { a.x,       a.y + a.h, z },
        { a.x + a.w, a.y + a.h, z }
    };
        
    v2 uv[6] = {0};

    /* Cool gradient:
    v4 c[6] = {
        { 0, 0, 1, 1 },
        { 1, 0, 1, 1 },
        { 0, 1, 1, 1 },
                    
        { 0, 0, 1, 1 },
        { 1, 0, 1, 1 },
        { 0, 1, 1, 1 }
    };
       
    v4 h[6] = {
        { 0, 0, 0.5, 1 },
        { 1, 0, 0.5, 1 },
        { 0, 1, 0.5, 1 },
                    
        { 0, 0, 0.5, 1 },
        { 1, 0, 0.5, 1 },
        { 0, 1, 0.5, 1 }
    };

    v4 p[6] = {
        { 0, 0, 0.5, 1 },
        { 0.5, 0, 1.0, 1 },
        { 0, 1, 0.5, 1 },
                    
        { 0, 0, 0.5, 1 },
        { 0.5, 0, 1.0, 1 },
        { 0, 1, 0.5, 1 }
    };
    */
    
    v4 c[6] = {
        { 0.05, 0.6, 0.3, 1 },
        { 0.05, 0.6, 0.3, 1 },
        { 0.05, 0.6, 0.3, 1 },
        
        { 0.05, 0.6, 0.3, 1 },
        { 0.05, 0.6, 0.3, 1 },
        { 0.05, 0.6, 0.3, 1 },
    };
    
    v4 h[6] = {
        { 0.00, 0.65, 0.225, 1 },
        { 0.00, 0.65, 0.225, 1 },
        { 0.00, 0.65, 0.225, 1 },
        
        { 0.00, 0.65, 0.225, 1 },
        { 0.00, 0.65, 0.225, 1 },
        { 0.00, 0.65, 0.225, 1 },
    };
    
    v4 p[6] = {
        { 0.00, 0.5, 0.20, 1 },
        { 0.00, 0.5, 0.20, 1 },
        { 0.00, 0.5, 0.20, 1 },
        
        { 0.00, 0.5, 0.20, 1 },
        { 0.00, 0.5, 0.20, 1 },
        { 0.00, 0.5, 0.20, 1 },
    };
    
    v4 d[6] = {
        { 0.30, 0.5, 0.35, 1 },
        { 0.30, 0.5, 0.35, 1 },
        { 0.30, 0.5, 0.35, 1 },
        
        { 0.30, 0.5, 0.35, 1 },
        { 0.30, 0.5, 0.35, 1 },
        { 0.30, 0.5, 0.35, 1 },
    };
    
    v4 s[6] = {
        { 0.00, 0.85, 0.35, 1 },
        { 0.00, 0.85, 0.35, 1 },
        { 0.00, 0.85, 0.35, 1 },
        
        { 0.00, 0.85, 0.35, 1 },
        { 0.00, 0.85, 0.35, 1 },
        { 0.00, 0.85, 0.35, 1 },
    };

    float tex[6] = {
        0, 0, 0,
        0, 0, 0
    };

    v4 *color = c;
    if(btn.disabled)             color = d;
    else if(btn.selected)        color = s;
    else if(btn.state & PRESSED) color = p;
    else if(btn.state & HOVERED) color = h;

    triangles(v, uv, color, tex, 6, gfx);
    draw_string_in_rect_centered(get_ui_string(btn.label, ui), a, FS_20, FONT_TITLE, {1, 1, 1, 1}, gfx);
}

void draw_textfield(UI_Element *e, UI_ID id, UI_Manager *ui, Graphics *gfx)
{
    _OPAQUE_UI_();    

    Assert(e->type == TEXTFIELD);
    auto *tf = &e->textfield;
    
    draw_rect(tf->a, {0.8, 0.8, 0.8, 1.0}, gfx);

    Rect inner_a = textfield_inner_rect(tf);
    
    Rect text_a = inner_a;
    text_a.y -= tf->scroll.value;
    
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
                      highlight_p0.y + bt.line_height * l,
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
                    inner_a.y - 4}, {4, 4}, {1, 0, 0, 0.5}, gfx);
        draw_rect_ps(inner_a.p + V2_X * (ui->active_textfield_state.last_vertical_nav_x-1), {2, inner_a.h}, {1, 0, 0, 0.5}, gfx);
#endif

#if true && DEBUG // DEBUG DISPLAY: CARET/HIGHLIGHT CODEPOINT INDEX

        { // CARET
            String cp_index_str = concat_tmp("", tf_state.caret.cp, gfx->debug.sb);
            float cp_index_str_w = string_width(cp_index_str, FS_12, FONT_TITLE, gfx);
        
            draw_rect_ps({inner_a.x - cp_index_str_w - 4, inner_a.y}, {cp_index_str_w + 4, 16}, {0, 0, 0, 1}, gfx);
            
            draw_string(cp_index_str, { inner_a.x - cp_index_str_w - 2, inner_a.y + 2 }, FS_12, FONT_TITLE, {1, 1, 1, 1}, gfx);   
        }

        { // HIGHLIGHT
            String cp_index_str = concat_tmp("", tf_state.highlight_start.cp, gfx->debug.sb);
            float cp_index_str_w = string_width(cp_index_str, FS_12, FONT_TITLE, gfx);
        
            draw_rect_ps({inner_a.x - cp_index_str_w - 4, inner_a.y + 16}, {cp_index_str_w + 4, 16}, {0.10, 0.61, 0.53, 1}, gfx);
            
            draw_string(cp_index_str, { inner_a.x - cp_index_str_w - 2, inner_a.y + 2 + 16 }, FS_12, FONT_TITLE, {1, 1, 1, 1}, gfx);   
        }
        
#endif

#endif
        
    }

    
#if false && DEBUG // DEBUG DISPLAY: INNER RECT
    {
        draw_rect_ps(inner_a.p, inner_a.s, {1, 1, 1, 0.5}, gfx);
    }
#endif

    draw_body_text(text, FS_16, FONT_INPUT, text_a, {0.1, 0.1, 0.1, 1}, gfx, false, &clip_rect, &bt);

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

    /* Cool gradient:
       v4 c[6] = {
       { 0, 0, 1, 1 },
       { 1, 0, 1, 1 },
       { 0, 1, 1, 1 },
                    
       { 0, 0, 1, 1 },
       { 1, 0, 1, 1 },
       { 0, 1, 1, 1 }
       };
       
       v4 h[6] = {
       { 0, 0, 0.5, 1 },
       { 1, 0, 0.5, 1 },
       { 0, 1, 0.5, 1 },
                    
       { 0, 0, 0.5, 1 },
       { 1, 0, 0.5, 1 },
       { 0, 1, 0.5, 1 }
       };

       v4 p[6] = {
       { 0, 0, 0.5, 1 },
       { 0.5, 0, 1.0, 1 },
       { 0, 1, 0.5, 1 },
                    
       { 0, 0, 0.5, 1 },
       { 0.5, 0, 1.0, 1 },
       { 0, 1, 0.5, 1 }
       };
    */
    
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

    v4 *color = c;
    if(slider->disabled)  color = d;

    triangles(v, uv, color, tex, 6, gfx);

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

void draw_room(Room *room, double t, bool translucent_pass, m4x4 projection, Graphics *gfx)
{
    v4 sand  = {0.6,  0.5,  0.4, 1.0f}; 
    v4 grass = {0.25, 0.6,  0.1, 1.0f};
    v4 stone = {0.42, 0.4, 0.35, 1.0f};
    v4 water = {0.1,  0.3, 0.5,  0.9f};

    auto *tiles = room->shared.tiles;

    const float tile_s = 1;

    auto *opaque_object_buffer = &gfx->world_render_buffer.opaque;
    if(!translucent_pass) {
        push_vertex_destination(VD_WORLD_OPAQUE, gfx);
        begin_vertex_render_object(opaque_object_buffer, M_IDENTITY);
    }

    
    for(int y = 0; y < room_size_y; y++) {
        for(int x = 0; x < room_size_x; x++) {
            
            Rect tile_a = { tile_s * x, tile_s * y, tile_s, tile_s };
            
            auto tile = tiles[y * room_size_x + x];
            
            v4 *color = NULL;
            float z = 0;
            switch(tile) {
                case TILE_SAND:  color = &sand; break;
                case TILE_GRASS: color = &grass; break;
                case TILE_STONE: color = &stone; break;
                case TILE_WATER: color = &stone; z = -0.5; break;
                default: Assert(false); break;
            }

            if(tile == TILE_WATER)
            {
                if(translucent_pass)
                {
                    v3 origin = {tile_a.x, tile_a.y, -0.18f};
                    float screen_z = vecmatmul_z(origin + V3(0.5, 0.5, 0), projection);
                    _TRANSLUCENT_WORLD_VERTEX_OBJECT_(M_IDENTITY, screen_z);


                    draw_quad(origin, {1, 0, 0},  {0, 1, 0}, water, gfx);

                    String str = STRING("WATER");
                    
                    m4x4 text_transform = rotation_matrix(axis_rotation(V3_X, PI/2.0f));    
                    text_transform = matmul(text_transform, scale_matrix({1.0f/string_width(str, FS_10, &gfx->fonts[FONT_TITLE]), 0, 1.0f/10.0f}));
                    for(int i = 0; i < 4; i++)
                    {
                        v3 text_p = origin;
                        text_p.z  = 1;

                        float zrot = i * (PI/2.0f);

                        text_p.y += (i == 1 || i == 2) ? 1.01f : -0.01f;
                        text_p.x += (i == 2 || i == 3) ? 1.01f : -0.01f;

                        m4x4 m = matmul(text_transform, rotation_matrix(axis_rotation(V3_Z, zrot)));
                        m = matmul(m, translation_matrix(text_p));
                        draw_string_3d(str, V2_ZERO, FS_10, FONT_TITLE, {1, 1, 1, 1}, m, gfx); // @Temporary @Norelease
                    }
                }
                else
                {
                    draw_quad({tile_a.x, tile_a.y,   z}, {1, 0, 0}, {0, 1, 0}, sand, gfx);
                    // WEST
                    if(x == 0 || tiles[y * room_size_x + x - 1] != TILE_WATER)
                        draw_quad({tile_a.x, tile_a.y,   z}, {0, 0,-z}, {0, 1, 0}, *color, gfx);
                    // NORTH
                    if(y == 0 || tiles[(y-1) * room_size_x + x] != TILE_WATER)
                        draw_quad({tile_a.x, tile_a.y,   z}, {0, 0,-z}, {1, 0, 0}, *color, gfx);
                    // EAST
                    if(x == room_size_x-1 || tiles[y * room_size_x + x + 1] != TILE_WATER)
                        draw_quad({tile_a.x+1, tile_a.y, z}, {0, 0,-z}, {0, 1, 0}, *color, gfx);
                    // SOUTH
                    if(y == room_size_y-1 || tiles[(y+1) * room_size_x + x] != TILE_WATER)
                        draw_quad({tile_a.x, tile_a.y+1, z}, {0, 0,-z}, {1, 0, 0}, *color, gfx);
                }
            }
            else if(color){
                draw_quad({tile_a.x, tile_a.y, z}, {1, 0, 0}, {0, 1, 0}, *color, gfx);
            }
        }
    }

    if(!translucent_pass) {
        end_vertex_render_object(opaque_object_buffer);
        pop_vertex_destination(gfx);
    }
}


void draw_world_view_background(UI_Element *e, Graphics *gfx)
{
    _OPAQUE_UI_();

    Assert(e->type == WORLD_VIEW);
    draw_rect(e->world_view.a, { 0.17, 0.15, 0.14, 1 }, gfx);
}

void draw_world(Room *room, double t, m4x4 projection, Graphics *gfx)
{
#if 1

    draw_room(room, t, false, projection, gfx);
    draw_room(room, t, true,  projection, gfx);    
    
#endif

#if 0
    // @Temporary
    {
        _TRANSLUCENT_WORLD_VERTEX_OBJECT_(rotation_around_point_matrix(axis_rotation(V3_X, PI + cos(t) * (PI/16.0) / 2.0), { room_size_x / 2.0f, room_size_y / 2.0f, 2.5 }), -1);
        draw_string(STRING("Abc"), V2_ZERO, FS_10, FONT_TITLE, {1, 1, 1, 1}, gfx);
    }
#endif
}


void no_checkin_draw_ground_pos(Ray rrray, v2 mouse_p, Graphics *gfx)
{
    { _OPAQUE_WORLD_VERTEX_OBJECT_(M_IDENTITY);
        v3 dir    = rrray.dir * 5.0f;
        v3 shadow = { dir.x, dir.y, 0 };


        
        draw_quad(V3_ZERO, dir,    normalize(cross(V3_Z, dir)) * 0.2f,
                  { 1, 0, 0, 1 }, gfx);
        
        draw_quad(V3_ZERO, shadow, normalize(cross(V3_Z, shadow)) * 0.2f,
                  { 0, 0, 0, 0.5f }, gfx);

        v3 ground_p = rrray.p0 + rrray.dir * (-rrray.p0.z / rrray.dir.z);
        
        if(ground_p.x >= 0 && ground_p.x < room_size_x &&
           ground_p.y >= 0 && ground_p.y < room_size_y)
        {
            v3 tile = ground_p - V3_XY * 0.5f;
            tile = { roundf(tile.x), roundf(tile.y), tile.z };
            
            draw_quad(tile + V3_Z * 0.01f, V3_X, V3_Y, { 1, 0, 0, 1 }, gfx);
            /*
            draw_quad(tile, V3_Z, V3_Y, { 0, 1, 0, 1 }, gfx);
            draw_quad(tile, V3_X, V3_Z, { 0, 0, 1, 1 }, gfx);
            */
        }
    }
}

DWORD render_loop(void *loop_)
{
    Render_Loop *loop = (Render_Loop *)loop_;

    Client     *client = loop->client;
    UI_Manager *ui;    
    Window     *main_window;
    
    Graphics gfx = {0};

    // START INITIALIZATION //
    lock_mutex(client->mutex);
    {
        Assert(loop->state == Render_Loop::INITIALIZING);
        
        ui =          &client->ui;    
        main_window = &client->main_window;
        
        gfx.fonts = client->fonts;
        
        bool graphics_init_result = init_graphics(main_window, &gfx);
        Assert(graphics_init_result);

        init_fonts(gfx.fonts, &gfx);

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


    // @Cleanup?
    UI_World_View *world_view = NULL;
    m4x4 world_projection;
    
    
    bool first_frame = true;
    u64 last_second = platform_get_time();
    
    while(true)
    {   
        lock_mutex(client->mutex);
        {
            
            triangles_this_frame = 0;
            double t = platform_get_time();
            
            if(loop->state == Render_Loop::SHOULD_EXIT) {
                unlock_mutex(client->mutex);
                break;
            }
                
            gpu_set_vsync_enabled(tweak_bool(TWEAK_VSYNC));
#if DEBUG
            draw_calls_last_frame = gfx.debug.num_draw_calls;
#endif

            frame_begin(main_window, first_frame, client->main_window_a.s, &gfx);

            u64 second = platform_get_time();

            const v4 background_color = { 0.3, 0.36, 0.42, 1 };
            draw_rect(rect(0, 0, gfx.frame_s.w, gfx.frame_s.h), background_color, &gfx);
                
            // Draw UI
            for(int i = ui->elements_in_depth_order.n-1; i >= 0; i--)
            {
                UI_ID id      = ui->elements_in_depth_order[i];
                UI_Element *e = find_ui_element(id, ui);
                Assert(e);

                switch(e->type) {
                    case WINDOW:   draw_window(e, ui, &gfx); break;
                    case UI_TEXT:  draw_ui_text(e, ui, &gfx); break;
                    case BUTTON:   draw_button(e, ui, &gfx); break;
                    case TEXTFIELD: draw_textfield(e, id, ui, &gfx); break;
                    case SLIDER:   draw_slider(e, &gfx);     break;
                    case DROPDOWN: draw_dropdown(e, &gfx); break;

                    case WORLD_VIEW: {
                        draw_world_view_background(e, &gfx);

                        if(world_view != NULL) {
                            Assert(false);
                            break;
                        }

                        m4x4 projection = e->world_view.camera.projection;
                        projection._23 += -0.1f + gfx.z_for_2d; // Translate in Z
                        
                        world_view = &e->world_view;
                        world_projection = projection;

                        draw_world(&client->game.room, t, projection, &gfx);

                        Ray rrray = e->world_view.mouse_ray;
                        
                        no_checkin_draw_ground_pos(rrray, e->world_view.mouse_p, &gfx);


                        gfx.z_for_2d -= 0.2;
                        
                    } break;
                        
                    default: Assert(false); break;
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

        
        for(int i = 0; i < ARRLEN(gfx.glyph_maps); i++)
        {
            update_sprite_map_texture_if_needed(&gfx.glyph_maps[i], &gfx);
        }

        gpu_set_depth_testing_enabled(true);
        gpu_set_depth_mask(true);
        gpu_clear_depth_buffer(); 

        // DEFAULT VERTEX BUFFER (@Temporary?)
        config_gpu_for_ui(&gfx); 
        {
            gpu_set_depth_mask(false);
            flush_vertex_buffer(&gfx.default_vertex_buffer, &gfx);
        }

        // OPAQUE UI //
        config_gpu_for_ui(&gfx); 
        {
            gpu_set_depth_mask(true);
            flush_render_object_buffer(&gfx.ui_render_buffer.opaque, false, &gfx);
        }

        // WORLD //
        if(world_view) {
            config_gpu_for_world(&gfx, world_view->a, world_projection);
            {       
                gpu_set_depth_mask(true);
                flush_render_object_buffer(&gfx.world_render_buffer.opaque, false, &gfx);

                gpu_set_depth_mask(false);
                flush_render_object_buffer(&gfx.world_render_buffer.translucent, true, &gfx);
            }
            world_view = NULL;
        }

        // TRANSLUCENT UI //
        config_gpu_for_ui(&gfx);
        {
            gpu_set_depth_mask(false);
            flush_render_object_buffer(&gfx.ui_render_buffer.translucent, true, &gfx);
        }
        
        
        Assert(current_vertex_buffer(&gfx) == &gfx.default_vertex_buffer);
        
        // Make sure we've flushed all vertex buffers //
        Assert(gfx.default_vertex_buffer.n == 0);
        Assert(gfx.world_render_buffer.opaque.vertices.n == 0);
        Assert(gfx.world_render_buffer.opaque.objects.n == 0);
        Assert(gfx.world_render_buffer.translucent.vertices.n == 0);
        Assert(gfx.world_render_buffer.translucent.objects.n == 0);
        //
        
        frame_end(main_window, &gfx);
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
