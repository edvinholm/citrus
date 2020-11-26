 

#define round(x) round(x)
#define log10(x) log10(x)
#define abs(x) abs(x)
#define fabs(x) fabs(x)
#define pow(x, y) pow(x, y)
#define sqrt(x) sqrt(x)
#define ln(x) log(x)
#define cos(x) cos(x)
#define sin(x) sin(x)
#define tan(x) tan(x)
#define atan(x, y) atan(x, y)
#define atan2(x, y) atan2(x, y)
#define Set_If_Greater(variable, value) variable = (value > variable) ? value : variable;
#define Set_If_Less(variable, value) variable = (value < variable) ? value : variable;

#if !(OS_WINDOWS)

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#else

#define max(a, b)  (((a) > (b)) ? (a) : (b))
#define min(a, b)  (((a) < (b)) ? (a) : (b))

#endif

    
#include "v2s.h"
#include "v2.cpp"
#include "v3s.h"
#include "v3.cpp"
#include "v4s.h"
#include "v4u.h"

v3 vecmatmul(m4x4 m, v3 v);

struct Quad
{
    v3 p0;
    v3 d1;
    v3 d2;
};


inline
Quad quad(v3 p0, v3 d1, v3 d2)
{
    Quad result;
    result.p0 = p0;
    result.d1 = d1;
    result.d2 = d2;
    return result;
}


#include "v4.cpp"
#include "v2s.cpp"
#include "v3s.cpp"
#include "v2u.cpp"

#include "rect.cpp"

enum Vector_Component
{
    COMP_X = 0,
    COMP_Y = 1,
    COMP_Z = 2,
    COMP_W = 3,
};

void vecmatmuls(m4x4 m, v3 *in, v3 *_out, u32 n);



struct Quat
{
    union
    {
        struct
        {
            union
            {
                struct
                {
                    float x, y, z;
                };
                v3 xyz;
            };
            float w;
        };
        v4 xyzw;
        float comp[4];
    };
};


const Quat Q_IDENTITY = {0, 0, 0, 1};


#include "matrix.cpp"



inline
bool operator == (Quat q, Quat r)
{
    return floats_equal(q.x, r.x) &&
        floats_equal(q.y, r.y) &&
        floats_equal(q.z, r.z) &&
        floats_equal(q.w, r.w);
}



inline
double deg_to_rad(double degrees)
{
    return degrees * 0.0174532925;
}

//I'm just guessing how this works.
inline
float euler_angle(Vector_Component axis, Quat q)
{
    return q.comp[axis] * q.w;
}

//NOTE: angle is in radians.
inline
float normalized_angle(double angle)
{
    int revolutions = angle / (double)PIx2;
    angle -= revolutions * (double)PIx2;
    return angle;
}

/*
inline
int abs(int x)
{
    return (x < 0) ? -x : x;
}
*/

template<typename T>
T clamp(T value, T min = 0, T max = 1)
{
    if(value < min) return min;
    if(value > max) return max;
    return value;
}

inline
float Lerp(float x, float y, float T)
{
    return x + (y-x)*T;
}


inline
bool IsPowerOfTwo(s32 x)
{
    return (x & (x - 1)) == 0;
}

inline
u32 NextPowerOfTwo(u32 x)
{
    while(!IsPowerOfTwo(x)) x++;
    return x;
}






inline
bool point_inside_circle(v2 p, v2 center, float radius)
{
    return (magnitude(p - center) <= radius);
}


float TriangleArea(float x1, float y1, float x2, float y2, float x3, float y3)
{
    return fabs((x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2))/2.0f);
}

//NOTE: A, B and C are the triangle's vertices. P is the point.
bool PointInsideTriangle(v2 P, v2 A, v2 B, v2 C)
{
    //yeeeez! This is from
    //        https://www.youtube.com/watch?v=hyAgJN3x4GA
    //and
    //https://github.com/SebLague/Gamedev-Maths/blob/master/PointInTriangle.cs
    
    float S1 = C.y - A.y;
    float S2 = C.x - A.x;
    float S3 = B.y - A.y;
    float S4 = P.y - A.y;
    
    float w1 = (A.x*S1 + S4*S2 - P.x*S1)/(S3*S2 - (B.x-A.x)*S1);
    float w2 = (S4 - w1*S3)/S1;
    return w1 >= 0 && w2 >= 0 && (w1+w2) <= 1;
    
}

bool PointInsideTriangle(float Px, float Py,
                         float Ax, float Ay, float Bx, float By, float Cx, float Cy)
{
    return PointInsideTriangle({ Px, Py }, { Ax, Ay },
                               { Bx, By }, { Cx, Cy });
}




inline
Quat quat(float x, float y, float z, float w)
{
    Quat q = {x, y, z, w};
    return q;
}

inline
Quat quat(v4 xyzw)
{
    Quat q;
    q.xyzw = xyzw;
    return q;
}


//NOTE: a is in radians
Quat axis_rotation(v3 axis, float a)
{
    return quat(sin(a/2.0f)*axis.x,
                sin(a/2.0f)*axis.y,
                sin(a/2.0f)*axis.z,
                cos(a/2.0f));
}

//NOTE: a is in radians
//NOTE: a will be normalized, only in the double version of this.
Quat axis_rotation(v3 axis, double a)
{
    return axis_rotation(axis, normalized_angle(a));
}


//NOTE: a is in degrees
inline
Quat axis_rotation_deg(v3 axis, float a)
{
    return axis_rotation(axis, deg_to_rad(a));
}


//NOTE: a is in degrees
//NOTE: a will be normalized, only in the double version of this.
inline
Quat axis_rotation_deg(v3 axis, double a)
{
    return axis_rotation(axis, normalized_angle(deg_to_rad(a)));
}


inline
Quat operator * (Quat q, Quat r)
{
    Quat result;
    /*
    result.x = q.w*r.x + q.x*r.w - q.y*r.z + q.z*r.y;
    result.y = q.w*r.y + q.x*r.z + q.y*r.w - q.z*r.x;
    result.z = q.w*r.z - q.x*r.y + q.y*r.x + q.z*r.w;
    result.w = q.w*r.w - q.x*r.x - q.y*r.y - q.z*r.z;
    */

    
    result.w = r.w * q.w - r.x * q.x - r.y * q.y - r.z * q.z;
    result.x = r.w * q.x + r.x * q.w + r.y * q.z - r.z * q.y;
    result.y = r.w * q.y - r.x * q.z + r.y * q.w + r.z * q.x;
    result.z = r.w * q.z + r.x * q.y - r.y * q.x + r.z * q.w;
    
    /*
    result.w = r.w*q.w - r.x*q.x - r.y*q.y - r.z*q.z;
    result.x = r.w*q.x + r.x*q.w + r.y*q.z - r.z*q.y;
    result.y = r.w*q.y - r.x*q.z + r.y*q.w + r.z*q.x;
    result.z = r.w*q.z + r.x*q.y - r.y*q.x + r.z*q.w;
    */
    
    return result;
}


inline
void operator *= (Quat &q, Quat r)
{
    q = q * r;
}

inline
Quat operator - (Quat q)
{
    return quat(q.x, q.y, q.z, -q.w);
}







inline
float rad_to_deg(float radians)
{
    return radians / 0.0174532925f;
}


inline
bool is_zero(float f)
{
    return (f >= -0.00001f && f <= 0.00001f);
}

inline
bool floats_equal(float f, float g)
{
    return is_zero(f-g);
}

inline
bool is_zero(double f)
{
    return (f >= -0.00001 && f <= 0.00001);
}

inline
bool doubles_equal(double f, double g)
{
    return is_zero(f-g);
}


