

const v2s V2S_ZERO = {0, 0};
const v2s V2S_ONE =  {1, 1};
const v2s V2S_X =    {1, 0};
const v2s V2S_Y =    {0, 1};
const v2s V2S_Z =    {0, 1};
const v2s V2S_XZ =   {1, 1};



inline
bool operator == (v2s u, v2s v)
{
    return u.x == v.x && u.y == v.y;
}

inline
v2 operator * (v2s u, float f)
{
    return { (float)u.x * f, (float)u.y * f };
}


inline
v2s V2S(s32 x, s32 y)
{
    return {x, y};
}

