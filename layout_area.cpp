
void push_area_layout(Rect a, Layout_Manager *manager)
{
    Layout layout;
    Zero(layout);
    layout.type = LAYOUT_AREA;

    layout.area.a = a;

    push_layout(layout, manager);
}


void set_area(Rect a, Layout_Manager *manager)
{
    Layout *layout = current_layout(manager);
    Assert(layout->type == LAYOUT_AREA);

    layout->area.a = a;
}

void set_area_x(float x, Layout_Manager *manager)
{
    Layout *layout = current_layout(manager);
    Assert(layout->type == LAYOUT_AREA);

    layout->area.a.x = x;
}

void set_area_y(float y, Layout_Manager *manager)
{
    Layout *layout = current_layout(manager);
    Assert(layout->type == LAYOUT_AREA);

    layout->area.a.y = y;
}

void set_area_size(v2 s, Layout_Manager *manager)
{
    Layout *layout = current_layout(manager);
    Assert(layout->type == LAYOUT_AREA);

    layout->area.a.s = s;
}

void set_area_height(float h, Layout_Manager *manager)
{
    Layout *layout = current_layout(manager);
    Assert(layout->type == LAYOUT_AREA);

    layout->area.a.h = h;
}

void set_area_width(float w, Layout_Manager *manager)
{
    Layout *layout = current_layout(manager);
    Assert(layout->type == LAYOUT_AREA);

    layout->area.a.w = w;
}


/////////////// ////////////////////// /////////////////////
/////////////// ////////////////////// /////////////////////
/////////////// ////////////////////// /////////////////////
/////////////// ////////////////////// /////////////////////


#define _AREA_(A) \
    push_area_layout(A, ctx.layout);                       \
    defer(pop_layout(ctx.layout););

#define _AREA_REL_(A) \
    push_area_layout(translated(A, area().p), ctx.layout);\
    defer(pop_layout(ctx.layout););

#define _AREA_COPY_() \
    _AREA_(area(ctx.layout));


#define _TOP_(H) \
    _AREA_(top_of(area(ctx.layout), H));

#define _BOTTOM_(H) \
    _AREA_(bottom_of(area(ctx.layout), H));

#define _LEFT_(W) \
    _AREA_(left_of(area(ctx.layout), W));

#define _RIGHT_(W) \
    _AREA_(right_of(area(ctx.layout), W));


#define _TOP_CENTER_(W, H) \
    _AREA_(center_top_of(area(ctx.layout), W, H));

#define _BOTTOM_CENTER_(W, H) \
    _AREA_(center_bottom_of(area(ctx.layout), W, H));

#define _LEFT_CENTER_(W, H) \
    _AREA_(center_left_of(area(ctx.layout), W, H));

#define _RIGHT_CENTER_(W, H) \
    _AREA_(center_right_of(area(ctx.layout), W, H));


#define _CENTER_X_(W) \
    _AREA_(center_x(area(ctx.layout), W));

#define _CENTER_Y_(H) \
    _AREA_(center_y(area(ctx.layout), H));

#define _CENTER_(W, H) \
    _AREA_(center_of(area(ctx.layout), W, H));


inline
Rect cut_top(float h, Layout_Manager *manager)
{   
    Rect a = area(manager);
    Rect result = cut_top_off(&a, h);
    set_area(a, manager);
    return result;
}

inline
Rect cut_bottom(float h, Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = cut_bottom_off(&a, h);
    set_area(a, manager);
    return result;
}

inline
Rect cut_left(float w, Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = cut_left_off(&a, w);
    set_area(a, manager);
    return result;
}

inline
Rect cut_right(float w, Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = cut_right_off(&a, w);
    set_area(a, manager);
    return result;
}

#define _TOP_CUT_(H) \
    _AREA_(cut_top(H, ctx.layout));

#define _BOTTOM_CUT_(H) \
    _AREA_(cut_bottom(H, ctx.layout));

#define _LEFT_CUT_(W) \
    _AREA_(cut_left(W, ctx.layout));

#define _RIGHT_CUT_(W) \
    _AREA_(cut_right(W, ctx.layout));


//NOTE: @BadName: "slide top" means take top_of(a, h) and translate a down by h.
//                Instead of doing a = removed_top(h) as we do in the "cut top" case.
inline
Rect slide_top(float h, Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = top_of(a, h);
    a.y -= h;
    set_area(a, manager);
    return result;
}

inline
Rect slide_bottom(float h, Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = bottom_of(a, h);
    a.y += h;
    set_area(a, manager);
    return result;
}

inline
Rect slide_left(float w, Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = left_of(a, w);
    a.x += w;
    set_area(a, manager);
    return result;
}

inline
Rect slide_right(float w, Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = right_of(a, w);
    a.x -= w;
    set_area(a, manager);
    return result;
}


#define _TOP_SLIDE_(H) \
    _AREA_(slide_top(H, ctx.layout));

#define _BOTTOM_SLIDE_(H) \
    _AREA_(slide_bottom(H, ctx.layout));

#define _LEFT_SLIDE_(W) \
    _AREA_(slide_left(W, ctx.layout));

#define _RIGHT_SLIDE_(W) \
    _AREA_(slide_right(W, ctx.layout));



#define _TOP_HALF_() \
    _AREA_(top_half_of(area(ctx.layout)));

#define _BOTTOM_HALF_() \
    _AREA_(bottom_half_of(area(ctx.layout)));

#define _LEFT_HALF_() \
    _AREA_(left_half_of(area(ctx.layout)));

#define _RIGHT_HALF_() \
    _AREA_(right_half_of(area(ctx.layout)));




#define _TOP_SQUARE_() \
    _AREA_(top_square_of(area(ctx.layout)));

#define _BOTTOM_SQUARE_() \
    _AREA_(bottom_square_of(area(ctx.layout)));

#define _LEFT_SQUARE_() \
    _AREA_(left_square_of(area(ctx.layout)));

#define _RIGHT_SQUARE_() \
    _AREA_(right_square_of(area(ctx.layout)));



inline
Rect cut_top_square(Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = cut_top_off(&a, a.w);
    set_area(a, manager);
    return result;
}

inline
Rect cut_bottom_square(Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = cut_bottom_off(&a, a.w);
    set_area(a, manager);
    return result;
}

inline
Rect cut_left_square(Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = cut_left_off(&a, a.h);
    set_area(a, manager);
    return result;
}

inline
Rect cut_right_square(Layout_Manager *manager)
{
    Rect a = area(manager);
    Rect result = cut_right_off(&a, a.h);
    set_area(a, manager);
    return result;
}



#define _TOP_SQUARE_CUT_() \
    _AREA_(cut_top_square(ctx.layout));

#define _BOTTOM_SQUARE_CUT_() \
    _AREA_(cut_bottom_square(ctx.layout));

#define _LEFT_SQUARE_CUT_() \
    _AREA_(cut_left_square(ctx.layout));

#define _RIGHT_SQUARE_CUT_() \
    _AREA_(cut_right_square(ctx.layout));



#define _REST_AFTER_TOP_SQUARE_() \
    _AREA_(removed_top_square(area()));

#define _REST_AFTER_BOTTOM_SQUARE_() \
    _AREA_(removed_bottom_square(area()));

#define _REST_AFTER_LEFT_SQUARE_() \
    _AREA_(removed_left_square(area()));

#define _REST_AFTER_RIGHT_SQUARE_() \
    _AREA_(removed_right_square(area()));



#define shrink(...) \
    set_area(shrunken(area(ctx.layout), __VA_ARGS__));

#define _SHRINK_(...) \
    _AREA_(shrunken(area(ctx.layout), __VA_ARGS__));


#define grow(...) \
    set_area(grown(area(), __VA_ARGS__));

#define grow_left(DW) \
    set_area(grown(area(), DW, 0, 0, 0));

#define grow_right(DW) \
    set_area(grown(area(), 0, DW, 0, 0));

#define grow_top(DH) \
    set_area(grown(area(), 0, 0, DH, 0));

#define grow_bottom(DH) \
    set_area(grown(area(), 0, 0, 0, DH));



#define _GROW_(...) \
    _AREA_(grown(area(ctx.layout), __VA_ARGS__));

#define _GROW_LEFT_(DW) \
    _AREA_(grown(area(ctx.layout), DW, 0, 0, 0));

#define _GROW_RIGHT_(DW) \
    _AREA_(grown(area(ctx.layout), 0, DW, 0, 0));

#define _GROW_TOP_(DH) \
    _AREA_(grown(area(ctx.layout), 0, 0, DH, 0));

#define _GROW_BOTTOM_(DH) \
    _AREA_(grown(area(ctx.layout), 0, 0, 0, DH));


#define _TRANSLATE_(...) \
    _AREA_(translated(area(ctx.layout), __VA_ARGS__));
