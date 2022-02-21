
#include "view_user.h"
#include "view_calculator.h"

enum View_Type
{
    VIEW_EMPTY = 0,
    VIEW_USER,
    VIEW_CALCULATOR,

    VIEW_NONE_OR_NUM
};

struct View_Address
{
    View_Type type;
    union {
        User_View_Address user;
        Calculator_View_Address calculator;
    };
};

struct Empty_View
{
    Array<u8, ALLOC_MALLOC> command_input;
};
void clear(Empty_View *empty)
{
    clear(&empty->command_input);
}

struct View
{
    View_Address address;
    union {
        Empty_View empty;
        // Here we can have view type specific state.
    };
};
void clear(View *view) {
    switch(view->address.type) {
        case VIEW_EMPTY: {
            clear(&view->empty);
        } break;

        default: break;
    }
}


