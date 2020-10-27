
struct Client
{
    Layout_Manager layout;
    UI_Manager ui;
    Input_Manager input;

    Window main_window;
};

// @Temporary
int fps = 0;
int frames_this_second = 0;
int ups = 0;
int updates_this_second = 0;



// nocheckin: Should this be here? Should it be called update_framebuffers?
bool update_framebuffers(Graphics *gfx)
{   
    // CREATE MULTISAMPLE TEXTURE //

    int max_num_samples = gpu_max_num_multisample_samples();
    int num_samples = min(16, max_num_samples);
    
    GPU_Texture_Parameters multisample_texture_params = {0};
    multisample_texture_params.pixel_components = GPU_PIX_COMP_RGBA;
    multisample_texture_params.pixel_format     = GPU_PIX_FORMAT_RGBA;
    multisample_texture_params.pixel_data_type  = GPU_PIX_DATA_UNSIGNED_BYTE;
    GPU_Error_Code gpu_error;
    if(!gpu_update_or_create_multisample_texture(&gfx->multisample_texture, (u64)gfx->frame_s.w, (u64)gfx->frame_s.h, num_samples, &gpu_error)) {
        Debug_Print("Failed to create multisample texture.\n");
        Assert(false);
        return false;
    }

    //NOTE: @Cleanup: We only need to attach the texture to the framebuffer once. Not every time we update the texture properties...
    gpu_update_framebuffer(gfx->multisample_framebuffer, gfx->multisample_texture, true);

    // nocheckin: Delete old fbo?
    // ////////////////////////// //

    return true;
}

void frame_begin(Window *window, bool first_frame, Graphics *gfx)
{
    platform_begin_frame(window);
    gpu_frame_init();

    u64 old_frame_w = (u64)gfx->frame_s.w;
    u64 old_frame_h = (u64)gfx->frame_s.h;

    float window_x, window_y;
    platform_get_window_rect(window, &window_x, &window_y, &gfx->frame_s.w, &gfx->frame_s.h);

    if(first_frame ||
       old_frame_w != (u64)gfx->frame_s.w || // @Robustness: Maybe frame w and h should be floats.
       old_frame_h != (u64)gfx->frame_s.h)
    {
        bool framebuffer_update_result = update_framebuffers(gfx);
        Assert(framebuffer_update_result);
    }
}

void frame_end(Window *window, Graphics *gfx)
{
    //nocheckin
    auto w = gfx->frame_s.w;
    auto h = gfx->frame_s.h;
    glBlitNamedFramebuffer(gfx->multisample_framebuffer, 0, 0, 0, w, h, 0, 0, w, h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    
    platform_end_frame(window);

    // Switch buffer set //
    gfx->buffer_set_index++;
    gfx->buffer_set_index %= ARRLEN(gfx->vertex_shader.buffer_sets);
    gpu_set_buffer_set(gfx->buffer_set_index, &gfx->vertex_shader);
    //--
}

bool init_graphics(Window *window, Graphics *gfx)
{    
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

        // INITIALIZATION DONE //        
        loop->state = Render_Loop::RUNNING;
    }
    unlock_mutex(loop->mutex);

    
    bool first_frame = true;
    u64 last_second = platform_milliseconds() / 1000;
    
    while(true)
    {
        u64 second = platform_milliseconds() / 1000;
        frame_begin(main_window, first_frame, &gfx);

        m4x4 ui_projection = make_m4x4(
            2.0/gfx.frame_s.w,  0, 0, -1,
            0, -2.0/gfx.frame_s.h, 0,  1,
            0,  0, 1,  0,
            0,  0, 0,  1);

        gpu_set_uniform_m4x4(gfx.vertex_shader.projection_uniform, ui_projection);
        gpu_set_uniform_m4x4(gfx.vertex_shader.transform_uniform,  M_IDENTITY);
            
        gpu_set_viewport(0, 0, gfx.frame_s.w, gfx.frame_s.h);
        
        lock_mutex(loop->mutex);
        {
            if(loop->state == Render_Loop::SHOULD_EXIT) {
                unlock_mutex(loop->mutex);
                break;
            }



                
            // Draw UI
            
            for(int i = 0; i < ui->elements.n; i++)
            {
                UI_ID id      = ui->element_ids[i];
                UI_Element *e = &ui->elements[i];

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

                {
                    triangles_now(v, uv, (btn.hovered) ? ((btn.pressed) ? p : h) : c, 6, &gfx);
                }
            }


            if(second != last_second) {
                fps = frames_this_second;
                frames_this_second = 0;
            }
            
            frames_this_second++;
            
        }
        unlock_mutex(loop->mutex);

        
        frame_end(main_window, &gfx);
        last_second = second;
        first_frame = false;
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

void client_ui(UI_Context ctx, Client *client)
{
    U(ctx);

    Rect a = area(ctx.layout);

    static int x = 2;

    _SHRINK_(10);
    _GRID_(x, x, 10);

    int new_x = x;
    for(int i = 0; i < x*x; i++)
    {
        _CELL_();
        if(button(PC(ctx, i))) {
            new_x++;
        }
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
    platform_create_gl_window(main_window, "Citrus", 960, 720);
    client_set_window_delegate(main_window, &client);
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
    //--

#if DEBUG // Debug stuff for keeping track of things like FPS and UPS.
    u64 last_second = 0;
    String_Builder sb = {0};
#endif
   
    while(true)
    {
        new_input_frame(input);
        if(!platform_process_input(main_window)) break;
        
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
        
        lock_mutex(render_loop.mutex);
        {   
            // BUILD UI //
            ui_build_begin(ui);
            {
            
                push_area_layout(window_a, layout);
            
                client_ui(P(ui_ctx), &client);
            
#if DEBUG
                if(second != last_second) {
                    ups = updates_this_second;
                    updates_this_second = 0;
                }
                updates_this_second++;
            
                platform_set_window_title(main_window, concat_cstring_tmp("Citrus | ", fps, " FPS | ", ups, " UPS", sb));
#endif
                pop_layout(layout);

            }
            ui_build_end(ui, &client.input);
            // //////// //
            
        }
        unlock_mutex(render_loop.mutex);
        
        last_second = second;

        // NOTE: Experienced stuttering when not sleeping here -- guessing that this thread kept locking the mutex without letting the render loop render its frame(s).
        platform_sleep_microseconds(100);
    }

    stop_render_loop(&render_loop);
 
    return 0;
}
