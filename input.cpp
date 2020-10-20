

struct Mouse
{
    v2 p;
    u8 buttons;
    u8 buttons_up;
    u8 buttons_down;
};

struct Input_Manager
{
    Mouse mouse;
};


void new_input_frame(Input_Manager *input)
{
    input->mouse.buttons_down = 0;
    input->mouse.buttons_up   = 0;
}
