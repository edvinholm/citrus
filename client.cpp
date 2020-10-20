
struct Client
{   
    Layout_Manager layout;
    UI_Manager ui;

    Window main_window;
};


#include "tmp.cpp"


DWORD render_loop(void *client_)
{
    Client *client = (Client *)client_;
    UI_Manager *ui = &client->ui;

    while(true)
    {
        Sleep(100);
        Debug_Print("render loop!\n");
        
        lock_mutex(ui->mutex);
        {
            Debug_Print("Draw UI.\n");
            // TODO "Draw" UI (Collect data and figure out what to draw)
        }
        unlock_mutex(ui->mutex);

        // TODO Actually draw things. (Send to graphics library, swap buffers...)
    }
}

int client_entry_point(int num_args, char **arguments)
{
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

    while(platform_process_input(&client.main_window))
    {
        ui_build_begin(ui);
        {
            {
                TIMED_BLOCK("Buttons");
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
                button(P(ui_ctx));
            }
            Sleep(200);
        }
        ui_build_end(ui);
    }
    

 
    return 0;
}
