
struct Client
{   
    Layout_Manager layout;
    UI_Manager ui;
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
    
    Thread render_loop_thread;
    if(!create_thread(&render_loop, &client, &render_loop_thread)) {
        Debug_Print("Unable to start render loop thread.\n");
        return 1;
    }

    while(true)
    {
        ui_build_begin(ui);
        {
            Debug_Print("UI build!\n");
            Sleep(200);
        }
        ui_build_end(ui);
    }
    

 
    return 0;
}
