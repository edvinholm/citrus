
// @Temporary
int fps = 0;
int frames_this_second = 0;
int ups = 0;
int updates_this_second = 0;
u64 draw_calls_last_frame = 0;


bool update_gpu_resources(Graphics *gfx)
{
    
    GPU_Texture_Parameters default_texture_params = {0};
    default_texture_params.pixel_components = GPU_PIX_COMP_RGBA;
    default_texture_params.pixel_format     = GPU_PIX_FORMAT_RGBA;
    default_texture_params.pixel_data_type  = GPU_PIX_DATA_UNSIGNED_BYTE;
    
    // GET MAX NUM MULTISAMPLE SAMPLES //
    int max_num_samples = gpu_max_num_multisample_samples();
    int num_samples = min(TWEAK_max_multisample_samples, max_num_samples);
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
    const v4 c_purple = { 0.3, 0, 0.9, 1 };
    quad(r, c_purple, gfx);

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
    auto old_color = gfx->current_color;
    gfx->current_color = {0.1, 0.1, 0.1, 1};
    {
        String text = get_ui_string(txt.text, ui);
        Rect clip_rect = a;
        draw_body_text(text, FS_12, FONT_BODY, a, gfx, false, &clip_rect);
    }
    gfx->current_color = old_color;
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
        
        bool graphics_init_result = init_graphics(main_window, &gfx);
        Assert(graphics_init_result);

        init_fonts(&gfx);

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

            for(int i = 0; i < ARRLEN(gfx.glyph_maps); i++)
            {
                update_sprite_map_texture_if_needed(&gfx.glyph_maps[i], &gfx);
            }

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
void bar_window(UI_Context ctx)
{
    U(ctx);

    _CENTER_X_(320);

    int c = 9;

    UI_ID window_id;
    { _AREA_(begin_window(P(ctx), &window_id, STRING("REFRIGERATOR")));

        _GRID_(c, c, 4);

        for(int i = 0; i < c * c; i++) {
            _CELL_();
            ui_text(STRING("But I must explain to you how all this mistaken idea of denouncing pleasure and praising pain was born and I will give you a complete account of the system, and expound the actual teachings of the great explorer of the truth, the master-builder of human happiness. No one rejects, dislikes, or avoids pleasure itself, because it is pleasure, but because those who do not know how to pursue pleasure rationally encounter consequences that are extremely painful. Nor again is there anyone who loves or pursues or desires to obtain pain of itself, because it is pain, but because occasionally circumstances occur in which toil and pain can procure him some great pleasure."), PC(ctx, i));
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
    UI_Button_State close_button_state;
    end_window(window_id, ctx.manager, &close_button_state);
    if(close_button_state & CLICKED_ENABLED)
        result = true;

    return result;
}

void client_ui(UI_Context ctx, Client *client)
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

        if(i == 2) bar_window(PC(ctx, i));
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

void client_set_window_delegate(Window *window, Client *client)
{
    Window_Delegate delegate = {0};
    delegate.data = client;
    
    delegate.key_down = NULL;
    delegate.key_up   = NULL;
    
    delegate.character_input = NULL;

    delegate.mouse_down = &client_mouse_down;
    delegate.mouse_up   = &client_mouse_up;
    delegate.mouse_move = &client_mouse_move;

    window->delegate = delegate;
}

// NOTE: Assumes *client has been zeroed.
void init_client(Client *client)
{
    init_ui_manager(&client->ui);
}

int client_entry_point(int num_args, char **arguments)
{    
    Debug_Print("I am a client.\n");

    // INIT CLIENT //
    Client client = {0};
    init_client(&client);
    Layout_Manager *layout = &client.layout;
    UI_Manager         *ui = &client.ui;
    Input_Manager   *input = &client.input;
    Window *main_window = &client.main_window;
    //--

    // CREATE WINDOW //
    platform_create_window(main_window, "Citrus", 1440, 1000, 72, 8);
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
    String_Builder sb = {0};
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
        
        lock_mutex(render_loop.mutex);
        {
            client.main_window_a = window_a;
            
            
            // BUILD UI //
            begin_ui_build(ui);
            {
                push_area_layout(window_a, layout);
            
                client_ui(P(ui_ctx), &client);
            
#if DEBUG || true
                if(second != last_second) {
                    ups = updates_this_second;
                    updates_this_second = 0;
                }
                updates_this_second++;
            
                platform_set_window_title(main_window, concat_cstring_tmp("Citrus | ", fps, " FPS | ", ups, " UPS | ", draw_calls_last_frame, " draws", sb));
#endif
                pop_layout(layout);

            }
            end_ui_build(ui, &client.input);
            // //////// //

            reset_temporary_memory();
        }
        unlock_mutex(render_loop.mutex);

#if DEBUG || true
        last_second = second;
#endif

        // NOTE: Experienced stuttering when not sleeping here -- guessing that this thread kept locking the mutex without letting the render loop render its frame(s).
        platform_sleep_microseconds(100);
    }

    stop_render_loop(&render_loop);
 
    return 0;
}
