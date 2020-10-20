
struct Client
{   
    Layout_Manager layout;
    UI_Manager ui;

    Window main_window;
};

// @Temporary
float clear_r = 0;
float clear_b = 0;

// @Temporary
int fps = 0;
int frames_this_second = 0;
int ups = 0;
int updates_this_second = 0;


void frame_begin(Window *window)
{
    platform_begin_frame(window);
    gpu_frame_init();
}

void frame_end(Window *window)
{
    platform_end_frame(window);
}

DWORD render_loop(void *client_)
{
    Client *client = (Client *)client_;
    UI_Manager *ui = &client->ui;
    
    Window *main_window = &client->main_window;

    
    if(!wglMakeCurrent(main_window->DeviceContext, main_window->GLContext)) {
        Debug_Print("wglMakeCurrent error: %d\n", GetLastError());
        Assert(false);
        return 1;
    }
    
    if(!gpu_init(1, 0, 1, 1)) {
        Assert(false);
        return 1;
    }

    u64 last_second = platform_milliseconds() / 1000;
    while(true)
    {
        u64 second = platform_milliseconds() / 1000;
        frame_begin(main_window);

        lock_mutex(ui->mutex);
        {
            glClearColor(clear_r, random_float() * 0.1, clear_b, 1); // nocheckin
            glClear(GL_COLOR_BUFFER_BIT); // nocheckin
            // TODO "Draw" UI (Collect data and figure out what to draw)

            if(second != last_second) {
                fps = frames_this_second;
                frames_this_second = 0;
            }
            
            frames_this_second++;
        }
        unlock_mutex(ui->mutex);

        // TODO Actually draw things. (Send to graphics library, swap buffers...)
        
        frame_end(main_window);
        last_second = second;
    }

    return 0;
}

int client_entry_point(int num_args, char **arguments)
{
    String_Builder sb = {0};
    
    Debug_Print("I am a client.\n");
    
    Client client = {0};
    UI_Manager *ui = &client.ui;

    platform_create_gl_window(&client.main_window, "Citrus");
    
    Thread render_loop_thread;
    if(!create_thread(&render_loop, &client, &render_loop_thread)) {
        Debug_Print("Unable to start render loop thread.\n");
        return 1;
    }

    UI_Context ui_ctx = UI_Context();
    ui_ctx.manager = ui;

    u64 last_second = 0;
        
    while(platform_process_input(&client.main_window))
    {
        u64 second = platform_milliseconds() / 1000;
            
        ui_build_begin(ui);
        {   
            if(second != last_second) {
                clear_r = random_float();
                clear_b = random_float();
            }
            
            TIMED_BLOCK("Buttons");
            button(P(ui_ctx));

            if(second != last_second) {
                ups = updates_this_second;
                updates_this_second = 0;
            }
            updates_this_second++;
            
            platform_set_window_title(&client.main_window, concat_tmp("Citrus | ", fps, " FPS | ", ups, " UPS", sb));
        }
        ui_build_end(ui);
            
        last_second = second;
    }
    

 
    return 0;
}
