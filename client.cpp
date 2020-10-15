
struct Client
{
    UI_Manager ui;
};


#include "tmp.cpp"

int client_entry_point(int num_args, char **arguments)
{
    Client client = {0};
    UI_Manager *ui = &client.ui;

    push_ui_location(__FILE__, __LINE__, 1, ui);
    push_ui_location(__FILE__, __LINE__, 1, ui);
    foo(ui);
    push_ui_location(__FILE__, __LINE__, 1, ui);
    push_ui_location(__FILE__, __LINE__, 1, ui);
    push_ui_location(__FILE__, __LINE__, 1, ui);
 
    Debug_Print("I am a client.\n");
    return 0;
}
