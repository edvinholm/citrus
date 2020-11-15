
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

    // UPDATE OR CREATE MULTISAMPLE TEXTURE //
    GPU_Error_Code gpu_error;
    if(!gpu_update_or_create_multisample_texture(&gfx->multisample_texture, (u64)gfx->frame_s.w, (u64)gfx->frame_s.h, num_samples, &gpu_error)) {
        Debug_Print("Failed to create multisample texture.\n");
        Assert(false);
        return false;
    }
    //

    // UPDATE MULTISAMPLE FRAMEBUFFER //
    //NOTE: @Cleanup: We only need to attach the texture to the framebuffer once. Not every time we update the texture properties...
    gpu_update_framebuffer(gfx->multisample_framebuffer, gfx->multisample_texture, true);
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


    gfx->vertex_buffer.n = 0;

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
    
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gfx->multisample_framebuffer);
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
    gpu_create_framebuffers(1, &gfx->multisample_framebuffer);


    return true;
}







struct Render_Loop
{
    enum State
    {
        INITIALIZING,
        RUNNING,
        SHOULD_EXIT
    };
    
    Mutex mutex;
    
    State  state;
    Thread thread;
    Client *client;
};


void quad(Rect a, v4 color, Graphics *gfx, Texture_ID texture = TEX_NONE_OR_NUM)
{
    v3 v[6] = {
        { a.x,       a.y,       0 },
        { a.x,       a.y + a.h, 0 },
        { a.x + a.w, a.y,       0 },

        { a.x + a.w, a.y,       0 },
        { a.x,       a.y + a.h, 0 },
        { a.x + a.w, a.y + a.h, 0 }
    };
    
    v2 uv[6] = {
        0, 0,
        0, 1,
        1, 0,

        1, 0,
        0, 1,
        1, 1
    };
    
    v4 c[6] = {
        color,
        color,
        color,
        
        color,
        color,
        color
    };


    float t = 0;

    if(texture != TEX_NONE_OR_NUM)
    {
        Assert(gfx->num_bound_textures > texture && gfx->bound_textures[texture] == texture);    
        t = (float)texture+1;
    }
             
    float tex[6] = {
        t, t, t,
        t, t, t
    };

    triangles(v, uv, c, tex, 6, gfx);
}

void draw_window(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
    Assert(e->type == WINDOW);
    auto *win = &e->window;
    auto a = win->current_a;

    const v4 shadow_c = { 0, 0, 0, 0.05 };
    quad(a, shadow_c, gfx);

    const float visible_border_w = 2;
    Rect r = shrunken(a, window_border_width-visible_border_w);
    
    quad(r, tweak_v4(TWEAK_WINDOW_BORDER_COLOR), gfx);

    r = shrunken(r, visible_border_w);

    gfx->current_color = {1, 1, 1, 1}; // @Temporary
    Rect title_a = cut_top_off(&r, window_title_height);
    const v4 c_white = { 1, 1, 1, 1 };
    draw_string_in_rect_centered(get_ui_string(win->title, ui), title_a, FS_20, FONT_TITLE, gfx);
    //---------

    quad(r, c_white, gfx);

    const v4 c_red    = { 1, 0, 0, 1 };
    if(win->resize_dir_x < 0) quad(left_of(  a, window_border_width), c_red, gfx);
    if(win->resize_dir_x > 0) quad(right_of( a, window_border_width), c_red, gfx);
    if(win->resize_dir_y < 0) quad(top_of(   a, window_border_width), c_red, gfx);
    if(win->resize_dir_y > 0) quad(bottom_of(a, window_border_width), c_red, gfx);

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
        quad(btn_a, btn_c, gfx);
    }
}


void draw_ui_text(UI_Element *e, UI_Manager *ui, Graphics *gfx)
{
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
    Assert(e->type == BUTTON);
    auto &btn = e->button;
    auto a = btn.a;

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
    draw_string_in_rect_centered(get_ui_string(btn.label, ui), a, FS_20, FONT_TITLE, gfx);
}

void draw_textfield(UI_Element *e, UI_ID id, UI_Manager *ui, Graphics *gfx)
{
    Assert(e->type == TEXTFIELD);
    auto *tf = &e->textfield;
    
    //TODO :PushPop color @Cleanup
    auto old_color = gfx->current_color;
    gfx->current_color = {0.8, 0.8, 0.8, 1.0};
    draw_rect(tf->a, gfx);

    String text = get_ui_string(tf->text, ui);

    Rect text_a = textfield_text_a(tf);
    Rect clip_rect = tf->a;

    Body_Text bt = create_textfield_body_text(text, text_a, gfx->fonts);

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
            
            // TODO :PushPop color
            gfx->current_color = {0.10, 0.61, 0.53, 1};
            draw_rect_ps({start_x - 2, highlight_p0.y + bt.line_height * l}, {end_x - start_x + 2, bt.line_height}, gfx);
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
    
        // TODO :PushPop color
        gfx->current_color = {0, 0, 0, 1};
        draw_rect_ps({caret_p.x - 2, caret_p.y}, {2, bt.line_height}, gfx);
        // ----------

#if DEBUG && true // DEBUG STUFF
        
#if true && DEBUG // DEBUG DISPLAY: LAST VERTICAL NAV X
        gfx->current_color = {1, 0, 0, 0.5};
        draw_rect_ps({text_a.x + ui->active_textfield_state.last_vertical_nav_x-2,
                    text_a.y - 4}, {4, 4}, gfx);
        draw_rect_ps(text_a.p + V2_X * (ui->active_textfield_state.last_vertical_nav_x-1), {2, text_a.h}, gfx);
#endif

        
#if true && DEBUG // DEBUG DISPLAY: CARET/HIGHLIGHT CODEPOINT INDEX

        { // CARET
            String cp_index_str = concat_tmp("", tf_state.caret.cp, gfx->debug.sb);
            float cp_index_str_w = string_width(cp_index_str, FS_12, FONT_TITLE, gfx);
        
            gfx->current_color = {0, 0, 0, 1};
            draw_rect_ps({text_a.x + text_a.w, text_a.y}, {cp_index_str_w + 4, 16}, gfx);
            
            gfx->current_color = {1, 1, 1, 1};
            draw_string(cp_index_str, { text_a.x + text_a.w + 2, text_a.y + 2 }, FS_12, FONT_TITLE, gfx);   
        }

        { // HIGHLIGHT
            String cp_index_str = concat_tmp("", tf_state.highlight_start.cp, gfx->debug.sb);
            float cp_index_str_w = string_width(cp_index_str, FS_12, FONT_TITLE, gfx);
        
            gfx->current_color = {0.10, 0.61, 0.53, 1};
            draw_rect_ps({text_a.x + text_a.w, text_a.y + 16}, {cp_index_str_w + 4, 16}, gfx);
            
            gfx->current_color = {1, 1, 1, 1};
            draw_string(cp_index_str, { text_a.x + text_a.w + 2, text_a.y + 2 + 16 }, FS_12, FONT_TITLE, gfx);   
        }
        
#endif

#endif
        
    }
    
    draw_body_text(text, FS_16, FONT_INPUT, text_a, {0.1, 0.1, 0.1, 1}, gfx, false, &clip_rect, &bt);

    // TODO :PushPop color
    gfx->current_color = old_color;
}

void draw_slider(UI_Element *e, Graphics *gfx)
{    
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
    
    quad(slider_handle_rect(a, slider->value), (slider->pressed) ? handle_c_p : handle_c, gfx);
}

void draw_dropdown(UI_Element *e, Graphics *gfx)
{
    Assert(e->type == DROPDOWN);
    auto *dd = &e->dropdown;
    
    quad(dropdown_rect(dd->box_a, dd->open), {0.8, 0.4f, 0.2f, 1.0f}, gfx);
}


DWORD render_loop(void *loop_)
{
    Render_Loop *loop = (Render_Loop *)loop_;

    Client     *client;
    UI_Manager *ui;    
    Window     *main_window;
    
    Graphics gfx = {0};

    // START INITIALIZATION //
    lock_mutex(loop->mutex);
    {
        Assert(loop->state == Render_Loop::INITIALIZING);
        
        client =      loop->client;
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
    unlock_mutex(loop->mutex);

    
    bool first_frame = true;
    u64 last_second = platform_milliseconds() / 1000;
    
    while(true)
    {   
        lock_mutex(loop->mutex);
        {
            if(loop->state == Render_Loop::SHOULD_EXIT) {
                unlock_mutex(loop->mutex);
                break;
            }
                
            gpu_set_vsync_enabled(tweak_bool(TWEAK_VSYNC));
            
#if DEBUG
            draw_calls_last_frame = gfx.debug.num_draw_calls;
#endif

            frame_begin(main_window, first_frame, client->main_window_a.s, &gfx);

            u64 second = platform_milliseconds() / 1000;
            
            // Window size things //
            m4x4 ui_projection = make_m4x4(
                2.0/gfx.frame_s.w,  0, 0, -1,
                0, -2.0/gfx.frame_s.h, 0,  1,
                0,  0, 1,  0,
                0,  0, 0,  1);

            gpu_set_uniform_m4x4(gfx.vertex_shader.projection_uniform, ui_projection);
            gpu_set_uniform_m4x4(gfx.vertex_shader.transform_uniform,  M_IDENTITY);
            
            gpu_set_viewport(0, 0, gfx.frame_s.w, gfx.frame_s.h);
            // //////// //

            const v4 background_color = { 0.3, 0.36, 0.42, 1 };
            quad(rect(0, 0, gfx.frame_s.w, gfx.frame_s.h), background_color, &gfx);
        
                
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
        unlock_mutex(loop->mutex);

        
        for(int i = 0; i < ARRLEN(gfx.glyph_maps); i++)
        {
            update_sprite_map_texture_if_needed(&gfx.glyph_maps[i], &gfx);
        }

        
        triangles_this_frame = gfx.vertex_buffer.n/3;
        
        triangles_now(gfx.vertex_buffer.p,
                      gfx.vertex_buffer.uv,
                      gfx.vertex_buffer.c,
                      gfx.vertex_buffer.tex,
                      gfx.vertex_buffer.n,
                      &gfx);
        
        frame_end(main_window, &gfx);
    }

    return 0;
}

// NOTE: Assumes you've zeroed *_ctx
bool start_render_loop(Render_Loop *_loop, Client *client)
{
    create_mutex(_loop->mutex);
    _loop->state = Render_Loop::INITIALIZING;
    _loop->client = client;

    // Start thread
    if(!create_thread(&render_loop, _loop, &_loop->thread)) {
        delete_mutex(_loop->mutex);
        return false;
    } 

    // Wait for the loop to initialize itself.
    while(true) {
        lock_mutex(_loop->mutex);
        defer(unlock_mutex(_loop->mutex););

        if(_loop->state != Render_Loop::INITIALIZING) {
            Assert(_loop->state == Render_Loop::RUNNING);
            break;
        }
    }

    return true;
}

void stop_render_loop(Render_Loop *loop)
{
    lock_mutex(loop->mutex);
    Assert(loop->state == Render_Loop::RUNNING);
    loop->state = Render_Loop::SHOULD_EXIT;
    unlock_mutex(loop->mutex);

    join_thread(loop->thread);

    delete_mutex(loop->mutex);
}


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

// @Temporary
bool foo_window(bool slider_disabled, UI_Context ctx)
{
    U(ctx);

    char *words[] = {
        "Moo",
        "OK",
        "Hello",
        "EGGPLANT",
        "CITRUS"
    };

    words[3] = ((int)(platform_milliseconds() / 1000.0f) % 3 == 0) ? "BUTTER" : "EGGPLANT";

    bool result = false;
    static int selected = -1;
    static float slider_value = 0.75f;

    UI_ID window_id;
    { _AREA_(begin_window(P(ctx), &window_id, STRING("FOO")));
        
        {_BOTTOM_CUT_(32);
            {_LEFT_HALF_();
                dropdown(P(ctx));
            }
            {_RIGHT_HALF_();
                slider_value = slider(slider_value, P(ctx));
            }   
        }
        cut_bottom(window_default_padding, ctx.layout);

        int rows_and_cols = 1 + round(slider_value * 5);
        
        { _GRID_(rows_and_cols, rows_and_cols, window_default_padding);
            for(int i = 0; i < rows_and_cols * rows_and_cols; i++) {
                _CELL_();
                
                if(button(PC(ctx, i), STRING(words[i % ARRLEN(words)]), (i % 2 > 0), (selected == i)) & CLICKED_ENABLED) {
                    selected = i;
                }
            }   
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

    static int x = 2;
    static int hidden = -1;

    _SHRINK_(10);
    _GRID_(x, x, 10);

    int new_x = x;
    for(int i = 0; i < x*x; i++)
    {   
        _CELL_();

        if(hidden == i) continue;

        if(i == 2) bar_window(PC(ctx, i), input);
        else if(foo_window((i % 2 == 0), PC(ctx, i))) hidden = i;
    }
    x = new_x;
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


int client_entry_point(int num_args, char **arguments)
{
    String_Builder sb = {0};
    Debug_Print("I am a client.\n");
    
    // INIT CLIENT //
    Client client = {0};
    Layout_Manager *layout = &client.layout;
    UI_Manager         *ui = &client.ui;
    Input_Manager   *input = &client.input;
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

    // START RENDER THREAD //
    Render_Loop render_loop = {0};
    if(!start_render_loop(&render_loop, &client))
    {
        Debug_Print("Unable to start render loop.\n");
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
        
        lock_mutex(render_loop.mutex);
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
            end_ui_build(ui, &client.input, client.fonts, &cursor);
            // //////// //

            reset_temporary_memory();
        }
        unlock_mutex(render_loop.mutex);

#if DEBUG || true
        last_second = second;
#endif
        
        platform_set_cursor_icon(cursor);

        // NOTE: Experienced stuttering when not sleeping here -- guessing that this thread kept locking the mutex without letting the render loop render its frame(s).
        platform_sleep_microseconds(100);
    }

    stop_render_loop(&render_loop);
 
    return 0;
}
