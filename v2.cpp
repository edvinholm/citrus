
struct v2
{
    union
    {
        struct
        {
            union
            {
                float x;
                float w;
            };
            union
            {
                float y;
                float h;
            };
        };
        float comp[2];
    };
};


const v2 V2_ZERO = {0, 0};
const v2 V2_X   =  {1, 0};
const v2 V2_Y   =  {0, 1};
const v2 V2_ONE =  {1, 1};
const v2 V2_XY  =  {1, 1};
const v2 V2_XY_NORM = { 0.7071067811865475244, 0.7071067811865475244 };


inline
v2 compmul(v2 a, v2 b)
{
    return { a.x * b.x, a.y * b.y };
}

inline
v2 compdiv(v2 a, v2 b)
{
    return { a.x / b.x, a.y / b.y };
}

inline
v2 compmin(v2 a, v2 b)
{
    return { min(a.x, b.x), min(a.y, b.y) };
}

inline
v2 compmax(v2 a, v2 b)
{
    return { max(a.x, b.x), max(a.y, b.y) };
}


inline
bool operator == (v2 A, v2 B)
{
    return floats_equal(A.x, B.x) && floats_equal(A.y, B.y);
}

inline
bool operator != (v2 a, v2 b)
{
    return !(a == b);
}

inline
v2 operator - (v2 A, v2 B)
{
    return {A.x - B.x, A.y - B.y};
}


inline
v2 operator - (v2 u)
{
    return {-u.x, -u.y};
}

inline
void operator -= (v2 &a, v2 b)
{
    a = a - b;
}


inline
v2 operator * (v2 u, float s)
{
    return { u.x * s, u.y * s };
}

inline
v2 operator * (float s, v2 u)
{
    return u * s;
}

inline
void operator*=(v2 &V, float S)
{
    V = V * S;
}


inline
v2 operator/(v2 A, float S)
{
    return {A.x/S, A.y/S};
}


inline
void operator/=(v2 &V, float S)
{
    V = V / S;
}

inline
v2 operator+(v2 V, v2 U)
{
    v2 Result = { V.x + U.x, V.y + U.y };
    return Result;
}

inline
void operator+=(v2 &V, v2 U)
{
    V = V + U;
}


inline
float magnitude(v2 v)
{
    return sqrtf(v.x*v.x + v.y*v.y);
}

inline
float magnitude_xy(float x, float y)
{
    return sqrtf(x*x + y*y);
}


inline
v2 normalize(v2 v)
{
    return v / magnitude(v);
}



inline
v2 compfloor(v2 u)
{
    return { floorf(u.x), floorf(u.y) };
}

inline
v2 compceil(v2 u)
{
    return { ceilf(u.x), ceilf(u.y) };
}

inline
v2 compround(v2 u)
{
    return { roundf(u.x), roundf(u.y) };
}



inline
float dot(v2 a, v2 b)
{
    return a.x * b.x + a.y * b.y;
}

