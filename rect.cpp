
struct Rect
{
    union
    {
        v2 p;
        struct
        {
            float x;
            float y;
        };
    };
    
    union
    {
        v2 s;
        struct
        {
            float w;
            float h;
        };
    };
};

bool equal(Rect a, Rect b) {
    return (floats_equal(a.x, b.x) &&
            floats_equal(a.y, b.y) &&
            floats_equal(a.w, b.w) &&
            floats_equal(a.h, b.h));
}


inline
bool operator == (Rect a, Rect b)
{
    return a.p == b.p && a.s == b.s;
}

inline
bool operator != (Rect a, Rect b)
{
    return !(a == b);
}


inline
Rect rect(float x, float y, float w, float h)
{
    return { x, y, w, h };
}

inline
Rect rect(v2 P, v2 S)
{
    Rect Result;
    Result.p = P;
    Result.s = S;
    return Result;
}

inline
float bottom_y(Rect a)
{
    return a.y;
}

inline
float right_x(Rect a)
{
    return a.x + a.w;
}

inline
float center_x(Rect A)
{
    return A.x + A.w/2.0f;
}

inline
float center_y(Rect A)
{
    return A.y + A.h/2.0f;
}


inline
Rect center_x(Rect a, float w)
{
    a.x += (a.w - w) / 2.0f;
    a.w = w;
    return a;
}

inline
Rect center_y(Rect a, float h)
{
    a.y += (a.h - h) / 2.0f;
    a.h = h;
    return a;
}


inline
v2 left_bottom_of(Rect a)
{
    return { a.x, a.y + a.h };
}

inline
v2 center_right_of(Rect a)
{
    return { right_x(a), center_y(a) };
}

inline
Rect center_right_of(Rect a, v2 s, float offset_from_right = 0)
{
    a.x += a.w - s.w - offset_from_right;
    a.y += (a.h - s.h) / 2.0f;
    a.s = s;
    return a;
}

inline
v2 center_left_of(Rect a)
{
    return { a.x, center_y(a) };
}


inline
v2 center_top_of(Rect a)
{
    return { center_x(a), a.y + a.h };
}

inline
Rect center_top_of(Rect a, float w, float h, float offset_from_top = 0)
{
    a.y += a.h - offset_from_top;
    a.x += (a.w - w) / 2.0f;
    a.w = w;
    a.h = h;
    return a;
}

inline
Rect center_top_of(Rect a, v2 s, float offset_from_top = 0)
{
    return center_top_of(a, s.w, s.h, offset_from_top);
}



inline
v2 center_bottom_of(Rect a)
{
    return { center_x(a), bottom_y(a) };
}

inline
Rect center_bottom_of(Rect a, v2 s, float offset_from_bottom = 0)
{
    a.y += offset_from_bottom;
    a.x += a.w / 2.0 - s.w / 2.0;
    return rect(a.p, s);
}



inline
v2 center_of(Rect a)
{
    return { center_x(a), center_y(a) };
}


inline
Rect center_of(Rect a, v2 size)
{
    a.p += a.s/2.0f - size / 2.0f;
    a.s = size;
    return a;
}

inline
Rect center_of(Rect a, float w, float h)
{
    return center_of(a, {w, h});
}


inline
Rect center_of(Rect a, v2 size, v2 offset)
{
    a.p += a.s/2.0f - size / 2.0f + offset;
    a.s = size;
    return a;
}


inline
Rect scaled(Rect a, float f)
{
    v2 new_s = a.s * f;
    
    a.x -= (new_s.w - a.w) / 2.0;
    a.y -= (new_s.h - a.h) / 2.0;
    
    a.s = new_s;

    return a;
}

inline
Rect translated(Rect a, v2 offset)
{
    return rect(a.p + offset, a.s);
}

inline
Rect shrunken(Rect a, float left, float right, float top, float bottom)
{
    return {a.x + left, a.y + bottom, a.w - left - right, a.h - top - bottom};
}

inline
Rect shrunken(Rect a, float inset)
{
    return shrunken(a, inset, inset, inset, inset);
}

inline
Rect shrunken(Rect a, v4 insets)
{
    return shrunken(a, insets.comp[0], insets.comp[1], insets.comp[2], insets.comp[3]);
}


inline
Rect grown(Rect a, float left, float right, float top, float bottom)
{
    return {a.x - left, a.y - bottom, a.w + left + right, a.h + top + bottom};
}

inline
Rect grown(Rect a, float inset)
{
    return grown(a, inset, inset, inset, inset);
}

inline
Rect grown(Rect a, v4 insets)
{
    return grown(a, insets.comp[0], insets.comp[1], insets.comp[2], insets.comp[3]);
}

inline
Rect round_rect(Rect a)
{
    v2 p = compround(a.p);
    v2 s = compround(a.s);
    return { p.x, p.y, s.x, s.y };
}

inline
Rect top_half_of(Rect a)
{
    a.h *= 0.5f;
    a.y += a.h;
    return a;
}

inline
Rect bottom_half_of(Rect a)
{
    a.h *= 0.5f;
    return a;
}

inline
Rect right_half_of(Rect a)
{
    a.w *= 0.5f;
    a.x += a.w;
    return a;
}


inline
Rect left_of(Rect Rect, float w)
{
    Rect.w = w;
    return Rect;
}

inline
Rect left_half_of(Rect a)
{
    a.w *= 0.5f;
    return a;
}

inline
Rect left_square_of(Rect a)
{
    return left_of(a, a.h);
}

inline
Rect right_of(Rect Rect, float w)
{
    Rect.x = Rect.x + Rect.w - w;
    Rect.w = w;
    return Rect;
}

inline
Rect right_square_of(Rect a)
{
    return right_of(a, a.h);
}

inline
Rect bottom_of(Rect a, float h)
{
    a.h = h;
    return a;
}

inline
Rect bottom_square_of(Rect a)
{
    return bottom_of(a, a.w);
}

inline
Rect top_of(Rect a, float h)
{
    a.y += a.h - h;
    a.h = h;
    return a;
}

inline
Rect top_square_of(Rect a)
{
    return top_of(a, a.w);
}

inline
v2 top_right_of(Rect a)
{
    return { a.x + a.w, a.y + a.h };
}


inline
Rect top_right_of(Rect a, float w, float h)
{
    a.x += a.w - w;
    a.y += a.h - h;
    a.w = w;
    a.h = h;
    return a;
}


inline
Rect bottom_right_of(Rect a, float w, float h)
{
    a.x = a.x + a.w - w;
    a.w = w;
    a.h = h;
    return a;
}


inline
v2 bottom_right_of(Rect a)
{
    return { a.x + a.w, a.y };
}


inline
Rect bottom_left_of(Rect a, float w, float h)
{
    a.x = a.x;
    a.w = w;
    a.h = h;
    return a;
}



inline
Rect removed_bottom(Rect a, float bottom)
{
    return { a.x, a.y + bottom, a.w, a.h - bottom };
}

inline
Rect removed_bottom_square(Rect a)
{
    return removed_bottom(a, a.w);
}


inline
Rect cut_bottom_off(Rect *a, float h) // TODO @Cleanup @BadNames everywhere dude
{
    Rect result = bottom_of(*a, h);
    *a = removed_bottom(*a, h);  //TODO  @Cleanup @BadNames everywhere dude
    return result;
}


inline
Rect removed_top(Rect a, float top)
{
    return { a.x, a.y, a.w, a.h - top };
}

inline
Rect removed_top_square(Rect a)
{
    return removed_top(a, a.w);
}

inline
Rect cut_top_off(Rect *a, float h) // TODO @Cleanup @BadNames everywhere dude
{
    Rect result = top_of(*a, h);
    *a = removed_top(*a, h);  //TODO  @Cleanup @BadNames everywhere dude
    return result;
}


inline
Rect removed_left(Rect Rect, float Left)
{
    return {
        Rect.x + Left, Rect.y,
        Rect.w - Left, Rect.h
    };
}

inline
Rect removed_left_square(Rect a)
{
    return removed_left(a, a.h);
}

Rect cut_left_off(Rect *a, float w) // TODO @Cleanup @BadNames everywhere dude
{
    Rect result = left_of(*a, w);
    *a = removed_left(*a, w);
    return result;
}

Rect cut_left_square_off(Rect *a)
{
    float w = a->h;
    return cut_left_off(a, w);
}



inline
Rect removed_right(Rect a, float right)
{
    return {
        a.x, a.y,
        a.w - right, a.h
    };
}

inline
Rect removed_right_square(Rect a)
{
    return removed_right(a, a.h);
}

Rect cut_right_off(Rect *a, float w) // TODO @Cleanup @BadNames everywhere dude
{
    Rect result = right_of(*a, w);
    *a = removed_right(*a, w);
    return result;
}


inline
Rect added_top(Rect a, float top)
{
    a.h += top;
    return a;
}


inline
Rect AddedLeft(Rect Rect, float Left)
{
    return {
        Rect.x - Left, Rect.y,
        Rect.w + Left, Rect.h
    };
}

inline
Rect added_bottom(Rect a, float bottom)
{
    a.y -= bottom;
    a.h += bottom;
    return a;
}

float ScaleToFitInRect(v2 ToFit, v2 RectSize)
{
    float Scale = 1.0f;
    if(ToFit.w > RectSize.w) Scale = RectSize.w/ToFit.w;
    if(ToFit.h * Scale > RectSize.h) Scale = RectSize.h/ToFit.h;
    return Scale;
}

Rect rect_around_point(v2 center, v2 size)
{
    return { center - size / 2.0, size };
}


inline
bool point_inside_rect(float px, float py, float rx, float ry, float rw, float rh)
{
    return (px >= rx && py >= ry &&
            px < rx + rw && py < ry + rh);
}

inline
bool point_inside_rect(v2 p, Rect a)
{
    return point_inside_rect(p.x, p.y, a.x, a.y, a.w, a.h);
}


inline
bool rect_inside_rect(Rect inner, Rect outer)
{
    v2 inner_p1 = inner.p + inner.s;
    v2 outer_p1 = outer.p + outer.s;

    bool x_inside = (inner.x >= outer.x && inner_p1.x <= outer_p1.x);
    bool y_inside = (inner.y >= outer.y && inner_p1.y <= outer_p1.y);

    return (x_inside && y_inside);
}


inline
bool rects_overlap(Rect a, Rect b)
{
    v2 a_p1 = a.p + a.s;
    v2 b_p1 = b.p + b.s;

    bool x_overlap = ((a.p.x < b_p1.x && a_p1.x >= b_p1.x) ||
                      (b.p.x < a_p1.x && b_p1.x >= a_p1.x));

    bool y_overlap = ((a.p.y < b_p1.y && a_p1.y >= b_p1.y) ||
                      (b.p.y < a_p1.y && b_p1.y >= a_p1.y));
    
    return (x_overlap && y_overlap);
}




inline
Rect rect_intersection(Rect A, Rect B)
{
    float a_x1 = A.x + A.w;
    float a_y1 = A.y + A.h;
    
    float b_x1 = B.x + B.w;
    float b_y1 = B.y + B.h;

    if(A.x >= b_x1 || A.y >= b_y1 ||
       B.x >= a_x1 || B.y >= a_y1)
        return {0, 0, 0, 0};

    float x0 = max(A.x, B.x);
    float y0 = max(A.y, B.y);

    float x1 = min(a_x1, b_x1);
    float y1 = min(a_y1, b_y1);

    float w = x1 - x0;
    float h = y1 - y0;

    Assert(w >= 0 && h >= 0);
    
    return { x0, y0, w, h };
}

Rect rect_union(Rect a, Rect b)
{
    v2 a_p1 = a.p + a.s;
    v2 b_p1 = b.p + b.s;
    v2 p1 = compmax(a_p1, b_p1);
    
    Rect result;
    result.x = min(a.x, b.x);
    result.y = min(a.y, b.y);
    result.s = p1 - result.p;

    return result;
}
