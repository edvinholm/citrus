




struct v2u
{
    union
    {
        struct
        {
            union
            {
                u32 x;
                u32 a;
                u32 w;
            };
            union
            {
                u32 y;
                u32 z;
                u32 b;
                u32 h;
            };
        };
        u32 comp[2];
    };
};





const v2u V2U_ZERO = {0, 0};
const v2u V2U_ONE =  {1, 1};
const v2u V2U_X =    {1, 0};
const v2u V2U_Y =    {0, 1};
const v2u V2U_Z =    {0, 1};
const v2u V2U_XZ =   {1, 1};


inline
v2u operator + (v2u u, v2u v)
{
    v2u result;
    result.x = u.x + v.x;
    result.y = u.y + v.y;
    return result;
}

inline
void operator += (v2u &u, v2u v)
{
    u = u + v;
}



inline
v2u operator - (v2u u, v2u v)
{
    v2u result;
    result.x = u.x - v.x;
    result.y = u.y - v.y;
    return result;
}



inline
void operator -= (v2u &u, v2u v)
{
    u = u - v;
}



inline
v2u operator * (v2u V, int I)
{
    v2u Result;
    Result.x = V.x * I;
    Result.y = V.y * I;
    return Result;
}

inline
v2 operator * (v2u V, float S)
{
    v2 FloatVector = {(float)V.x, (float)V.y};
    FloatVector *= S;
    return FloatVector;
}


inline
v2u V2U(u32 x, u32 y)
{
    v2u Result;
    Result.x = x;
    Result.y = y;
    return Result;
}

inline
v2u V2U(v2 V)
{
    return V2U((u32)V.x, (u32)V.y);
}

inline
v2u V2U(v2s u)
{
    v2u v;
    v.x = u.x;
    v.y = u.y;
    return v;
}


inline
bool operator == (v2u u, v2u v)
{
    return u.x == v.x && u.y == v.y;
}
