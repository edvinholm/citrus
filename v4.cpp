
struct v4
{
    union
    {
        struct
        {
            union
            {
                v3 rgb;
                v3 xyz;
                struct
                {
                    union
                    {
                        v2 xy;
                        struct
                        {
                            union
                            {
                                float x;
                                float r;
                                float left; //Insets, paddings etc.
                            };
                            union
                            {
                                float y;
                                float g;
                                float right; //Insets, paddings etc.
                            };
                            union
                            {
                                float z;
                                float b;
                                float top; //Insets, paddings etc.
                            };
                        };
                    };
                };
            };
            union
            {
                float w;
                float a;
                float bottom; //Insets, paddings etc.
            };
        };
        float comp[4];
    };
};

const v4 V4_ZERO = {0, 0, 0, 0};
const v4 V4_ONE = {1, 1, 1, 1};
const v4 V4_X = {1, 0, 0, 0};
const v4 V4_Y = {0, 1, 0, 0};
const v4 V4_Z = {0, 0, 1, 0};
const v4 V4_W = {0, 0, 0, 1};

const v4 V4_R = V4_X;
const v4 V4_G = V4_Y;
const v4 V4_B = V4_Z;
const v4 V4_A = V4_W;



inline
v4 V4(float x, float y, float z, float w = 1.0f)
{
    v4 V = { x, y, z, w };
    return V;
}

inline
v4 V4(v3 v, float w = 0.0f)
{
    v4 result;
    result.xyz = v;
    result.w = w;
    return result;
}


inline
v4 operator * (v4 V, float S)
{
    V.x *= S;
    V.y *= S;
    V.z *= S;
    V.w *= S;
    return V;
}


inline
v4 operator * (float s, v4 u)
{
    return u * s;
}


inline
void operator *= (v4 &V, float S)
{
    V = V * S;
}



v4 operator -(v4 v)
{
    return V4(-v.x, -v.y, -v.z, -v.w);
}

inline
v4 operator -(v4 u, v4 v)
{
    return V4(u.x - v.x, u.y - v.y, u.z - v.z, u.w - v.w);
}

inline
v4 operator +(v4 u, v4 v)
{
    return V4(u.x + v.x, u.y + v.y, u.z + v.z, u.w + v.w);
}



inline
void operator += (v4 &u, v4 v)
{
    u = u + v;
}


inline
void operator -= (v4 &u, v4 v)
{
    u = u - v;
}


inline
bool operator == (v4 u, v4 v)
{
    return floats_equal(u.x, v.x) && floats_equal(u.y, v.y) && floats_equal(u.z, v.z) && floats_equal(u.w, v.w);
}

inline
bool operator != (v4 u, v4 v)
{
    return !(u == v);
}



inline
float magnitude(v4 v)
{
    return sqrt(pow(v.x, 2) + pow(v.y, 2) + pow(v.z, 2) + pow(v.w, 2));
}


inline
v4 normalized(v4 v)
{
    float mag = magnitude(v);
    if(mag <= 0.0f) return V4_ZERO;
    return v * (1.0f/mag);
}
