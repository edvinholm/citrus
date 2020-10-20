
struct Client
{   
    Layout_Manager layout;
    UI_Manager ui;

    Window main_window;

    // @Temporary!!!!!!!!!
    v2 mouse_p;
};

// @Temporary
int fps = 0;
int frames_this_second = 0;
int ups = 0;
int updates_this_second = 0;



void frame_begin(Window *window, Graphics *gfx)
{
    platform_begin_frame(window);
    gpu_frame_init();

    float window_x, window_y;
    platform_get_window_rect(window, &window_x, &window_y, &gfx->frame_s.w, &gfx->frame_s.h);
}

void frame_end(Window *window, Graphics *gfx)
{
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
        return false;
    }
    else {
        Debug_Print("Shaders compiled successfully.\n");
    }
#endif
    
    if(!gpu_init_shaders(&gfx->vertex_shader, &gfx->fragment_shader, &gfx->gpu_ctx)) {
        Debug_Print("Failed to init shaders.\n");
        return false;
    }

    return true;
}

DWORD render_loop(void *client_)
{
    // nocheckin
    // TODO: @ThreadSafety Other threads should wait for this to init, because it uses things like temporary memory....
    
    Client *client = (Client *)client_;
    UI_Manager *ui = &client->ui;    
    Window *main_window = &client->main_window;

    Graphics gfx = {0};

    bool graphics_init_result = init_graphics(main_window, &gfx);
    Assert(graphics_init_result);

    u64 last_second = platform_milliseconds() / 1000;
    while(true)
    {
        u64 second = platform_milliseconds() / 1000;
        frame_begin(main_window, &gfx);

        m4x4 ui_projection = make_m4x4(
            2.0/gfx.frame_s.w,  0, 0, -1,
            0, -2.0/gfx.frame_s.h, 0,  1,
            0,  0, 1,  0,
            0,  0, 0,  1);

        gpu_set_uniform_m4x4(gfx.vertex_shader.projection_uniform, ui_projection);
        gpu_set_uniform_m4x4(gfx.vertex_shader.transform_uniform,  M_IDENTITY);
            
        gpu_set_viewport(0, 0, gfx.frame_s.w, gfx.frame_s.h);
        
        lock_mutex(ui->mutex);
        {
            // Draw UI
            
            // nocheckin
            v2 asum = {0};
            
            for(int i = 0; i < ui->elements.n; i++)
            {
                UI_ID id      = ui->element_ids[i];
                UI_Element *e = &ui->elements[i];

                Assert(e->type == BUTTON);
                auto &btn = e->button;
                auto a = btn.a;

                // nocheckin
                switch(i) {
                    case 0: asum.w += a.w; asum.h += a.h; break;
                    case 1:                asum.h += a.h; break;
                    case 2: asum.w += a.w;                break;
                    case 3:                               break;
                    default: Assert(false); break;
                }

                v3 v[6] = {
                    { a.x,       a.y,       0 },
                    { a.x,       a.y + a.h, 0 },
                    { a.x + a.w, a.y,       0 },

                    { a.x + a.w, a.y,       0 },
                    { a.x,       a.y + a.h, 0 },
                    { a.x + a.w, a.y + a.h, 0 }
                };
        
                v2 uv[6] = {0};

                v4 m_c[6] = {
                    { 0, 0, 1, 1 },
                    { 1, 0, 1, 1 },
                    { 0, 1, 1, 1 },
                    
                    { 0, 0, 1, 1 },
                    { 1, 0, 1, 1 },
                    { 0, 1, 1, 1 }
                };
                    
                v4 n_c[6] = {
                    { 0, 0, 0, 1 },
                    { 1, 0, 0, 1 },
                    { 0, 1, 0, 1 },
                    
                    { 0, 0, 0, 1 },
                    { 1, 0, 0, 1 },
                    { 0, 1, 0, 1 }
                };

                {
                    triangles_now(v, uv, (btn.marked) ? m_c : n_c, 6, &gfx);
                }
            }


            if(second != last_second) {
                fps = frames_this_second;
                frames_this_second = 0;
            }
            
            frames_this_second++;
            
        }
        unlock_mutex(ui->mutex);

        // Draw other stuff here
        
        frame_end(main_window, &gfx);
        last_second = second;
    }

    return 0;
}


void client_ui(UI_Context ctx, Client *client)
{
    U(ctx);
    
    v2 mouse_p = client->mouse_p;

    { _LEFT_CUT_(mouse_p.x);
        { _TOP_CUT_(mouse_p.y); button(P(ctx)); }
        {                       button(P(ctx)); }
    }
    {
        { _TOP_CUT_(mouse_p.y); button(P(ctx)); }
        {                       button(P(ctx)); }
    }
}


void client_mouse_move(Window *window, int x, int y, u64 ms, void *client_)
{
    auto *client = (Client *)client_;
    
    client->mouse_p = { (float)x, (float)y };
}

void client_set_window_delegate(Window *window, Client *client)
{
    Window_Delegate delegate = {0};
    delegate.data = client;
    
    delegate.key_down = NULL;
    delegate.key_up   = NULL;
    
    delegate.character_input = NULL;

    delegate.mouse_down = NULL;
    delegate.mouse_up   = NULL;
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
    UI_Manager     *ui = &client.ui;    
    Window *main_window = &client.main_window;
    //--

    // CREATE WINDOW //
    platform_create_gl_window(main_window, "Citrus", 960, 720);
    client_set_window_delegate(main_window, &client);
    //--

    // START RENDER THREAD //
    Thread render_loop_thread;
    if(!create_thread(&render_loop, &client, &render_loop_thread)) {
        Debug_Print("Unable to start render loop thread.\n");
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
   
    while(platform_process_input(main_window))
    {   
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
            
            platform_set_window_title(main_window, concat_tmp("Citrus | ", fps, " FPS | ", ups, " UPS", sb));
#endif
            pop_layout(layout);

        }
        ui_build_end(ui);
        // //////// //
            
        last_second = second;

        // NOTE: Experienced stuttering when not sleeping here -- guessing that this thread kept locking the mutex without letting the render loop render its frame(s).
        platform_sleep_microseconds(100);
    }
 
    return 0;
}
