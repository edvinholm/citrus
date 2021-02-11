

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
    
    // IMPORTANT: It is forbidden to put a \b after a non-\b character here.
    //            But you can put non-\b characters after \b characters.
    //            In other words, if there are \b's in here, they have to be
    //            at the beginning.
    //            If the user inputs non-\b characters followed by a \b,
    //            the last character should be removed instead of adding a \b.
    Array<u8, ALLOC_MALLOC> text; // NOTE: This is UTF-8 encoded.

    Array<virtual_key, ALLOC_MALLOC> keys; // NOT reset every loop.
    Array<virtual_key, ALLOC_MALLOC> keys_down; 
    Array<virtual_key, ALLOC_MALLOC> keys_up;
    Array<virtual_key, ALLOC_MALLOC> key_hits; // Keys are added here for every hit (repeats included). Reset every loop.
};


void new_input_frame(Input_Manager *input)
{
    input->mouse.buttons_down = 0;
    input->mouse.buttons_up   = 0;

    input->text.n = 0;

    input->keys_down.n = 0;
    input->keys_up.n   = 0;
    input->key_hits.n  = 0;
}
