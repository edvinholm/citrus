
v3s V3S(s32 x, s32 y, s32 z);

struct v3
{
    union
    {
        struct
        {
            union
            {
                struct
                {
                    union
                    {
                        float x;
                        float r;
                        float w;
                    };
                    union
                    {
                        float y;
                        float g;
                        float h;
                    };
                };
                v2 xy;
                v2 rg;
                v2 wh;
            };
            union
            {
                float z;
                float b;
                float d;
            };
        };
        float comp[3];
    };
};


const v3 V3_ZERO = {0, 0, 0};
const v3 V3_ONE = {1, 1, 1};
const v3 V3_X = {1, 0, 0};
const v3 V3_Y = {0, 1, 0};
const v3 V3_Z = {0, 0, 1};
const v3 V3_XZ = {1, 0, 1};
const v3 V3_XY = {1, 1, 0};
const v3 V3_MIN = {FLT_MIN, FLT_MIN, FLT_MIN};
const v3 V3_MAX = {FLT_MAX, FLT_MAX, FLT_MAX};


inline
v3 V3(float x, float y, float z)
{ 
    v3 result;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}


inline
v3 V3(v2 v, float z = 0.0f)
{ 
    v3 Result;
    Result.x = v.x;
    Result.y = v.y;
    Result.z = z;
    return Result;
}

inline
bool operator == (v3 u, v3 v)
{
    return floats_equal(u.x, v.x) && floats_equal(u.y, v.y) && floats_equal(u.z, v.z);
}


inline
v3 cross(v3 a, v3 b)
{
    v3 result;
    result.x = a.y*b.z - a.z*b.y;
    result.y = a.z*b.x - a.x*b.z;
    result.z = a.x*b.y - a.y*b.x;
    return result;
}

inline
float dot(v3 a, v3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}


inline
v3 operator - (v3 u, v3 v)
{
    return V3(u.x - v.x, u.y - v.y, u.z - v.z);
}


inline
void operator-=(v3 &V, v3 U)
{
    V = V - U;
}



void operator *= (v3 &V, float S)
{
    V.x *= S;
    V.y *= S;
    V.z *= S;
}

void operator *= (v3 &u, v3 v)
{
    u.x *= v.x;
    u.y *= v.y;
    u.z *= v.z;
}

v3 operator * (v3 v, float s)
{
    return {v.x*s, v.y*s, v.z*s};
}

v3 operator * (float s, v3 v)
{
    return v * s;
}



v3 operator -(v3 v)
{
    return V3(-v.x, -v.y, -v.z);
}

v3 operator + (v3 v, v3 u)
{
#if 0 // SIMD version (x86)
    __m128 a = _mm_set_ps(v.x, v.y, v.z, 0);
    __m128 b = _mm_set_ps(u.x, u.y, u.z, 0);

    __m128 sum = _mm_add_ps(a, b);

    float result[4];
    _mm_store_ps(result, sum);
    
    return { result[3], result[2], result[1] };

#else // non-SIMD version
    return { v.x + u.x, v.y + u.y, v.z + u.z };
#endif
    
}

v3 operator + (v3 u, v2 v)
{
    return { u.x + v.x, u.y + v.y, u.z };
}

void operator += (v3 &v, v3 u)
{
    v = v + u;
}



v3 compmul(v3 a, v3 b) {
    return { a.x * b.x, a.y * b.y, a.z * b.z };
}

v3 compdiv(v3 a, v3 b) {
    return { a.x / b.x, a.y / b.y, a.z / b.z };
}

v3 compmin(v3 a, v3 b) {
    return { min(a.x, b.x), min(a.y, b.y), min(a.z, b.z) };
}

v3 compmax(v3 a, v3 b) {
    return { max(a.x, b.x), max(a.y, b.y), max(a.z, b.z) };
}

v3 compfloor(v3 u) {
    return { floorf(u.x), floorf(u.y), floorf(u.z) };
}

v3 compceil(v3 u) {
    return { ceilf(u.x), ceilf(u.y), ceilf(u.z) };
}

v3 compround(v3 u) {
    return { roundf(u.x), roundf(u.y), roundf(u.z) };
}

v3 compabs(v3 u) {
    return { (float)fabs(u.x), (float)fabs(u.y), (float)fabs(u.z) };
}



inline
float magnitude(v3 v)
{
    return sqrt(pow(v.x, 2) + pow(v.y, 2) + pow(v.z, 2));
}


inline
v3 normalize(v3 v)
{
    float mag = magnitude(v);
    if(mag <= 0.0f) return V3_ZERO;
    return v * (1.0f/magnitude(v));
}


void magnitude_and_direction(v3 v, float *_magnitude, v3 *_direction)
{
    float mag = magnitude(v);
    
    if(mag <= 0.0f) *_direction = V3_ZERO;
    else            *_direction = v * (1.0f / mag);

    *_magnitude = mag;
}
