

const v3s V3S_ZERO = {0, 0, 0};
const v3s V3S_ONE =  {1, 1, 1};
const v3s V3S_X =    {1, 0, 0};
const v3s V3S_Y =    {0, 1, 0};
const v3s V3S_Z =    {0, 0, 1};
const v3s V3S_XZ =   {1, 0, 1};


inline
v3s round_v3_to_v3s(v3 v)
{
    v3s result;
    result.x = round(v.x);
    result.y = round(v.y);
    result.z = round(v.z);
    return result;
}

inline
v3s v3s_abs(v3s v)
{
    v.x = abs(v.x);
    v.y = abs(v.y);
    v.z = abs(v.z);
    return v;
}

inline
v3s V3S(v3 v)
{
    v3s result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    return result;
}


inline
v3s V3S(s32 x, s32 y, s32 z)
{
    v3s result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

inline
v3 V3(v3s v)
{ 
    v3 result;
    result.x = v.x;
    result.y = v.y;
    result.z = v.z;
    return result;
}


inline
v3s operator + (v3s u, v3s v)
{
    v3s result;
    result.x = u.x + v.x;
    result.y = u.y + v.y;
    result.z = u.z + v.z;
    return result;
}




inline
void operator += (v3s &u, v3s v)
{
    u = u + v;
}



inline
v3s operator - (v3s u, v3s v)
{
    v3s result;
    result.x = u.x - v.x;
    result.y = u.y - v.y;
    result.z = u.z - v.z;
    return result;
}


inline
void operator -= (v3s &u, v3s v)
{
    u = u - v;
}


inline
v3s operator * (v3s u, int i)
{
    v3s result;
    result.x = u.x * i;
    result.y = u.y * i;
    result.z = u.z * i;
    return result;
}


inline
v3s operator / (v3s u, int i)
{
    v3s result;
    result.x = u.x / i;
    result.y = u.y / i;
    result.z = u.z / i;
    return result;
}


inline
bool operator == (v3s u, v3s v)
{
    return u.x == v.x && u.y == v.y && u.z == v.z;
}

inline
bool operator != (v3s u, v3s v)
{
    return !(u == v);
}

inline
v3s operator * (v3s v, u32 i)
{
    v3s Result;
    Result.x = v.x * i;
    Result.y = v.y * i;
    Result.z = v.z * i;
    return Result;
}


