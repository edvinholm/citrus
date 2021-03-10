 

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

v3 vecmatmul(v3 v, m4x4 m, float w = 1.0f);


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

void vecmatmuls(v3 *in, m4x4 m, v3 *_out, u32 n);



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


struct Ray
{
    v3 p0;
    v3 dir;
};


struct AABB
{
    v3 p;
    v3 s;
};


struct Quad
{
    v3 p;
    v2 s;
    Quat q;
};


inline
Quad quad(v3 p, v2 s, Quat q = Q_IDENTITY)
{
    Quad result;
    result.p = p;
    result.s = s;
    result.q = q;
    return result;
}

Quad quad(Rect a, float z = 0, Quat q = Q_IDENTITY)
{
    Quad quad = {0};
    
    quad.p = { a.x, a.y, z };
    quad.s = a.s;
    quad.q = q;

    return quad;
}

Quad center_of(Quad a, v2 s)
{
    v3 e_x = rotate_vector(V3_X, a.q);
    v3 e_y = rotate_vector(V3_Y, a.q);

    a.p += e_x * ((a.s.x - s.x) * 0.5f);
    a.p += e_y * ((a.s.y - s.y) * 0.5f);

    a.s -= s;

    return a;
}

v3 center_of(Quad a)
{
    v3 e_x = rotate_vector(V3_X, a.q);
    v3 e_y = rotate_vector(V3_Y, a.q);

    v3 p = a.p;
    p += e_x * (a.s.x/2.0f);
    p += e_y * (a.s.y/2.0f);

    return p;
}

Quad bottom_of(Quad a, float h)
{
    v3 e_y = rotate_vector(V3_Y, a.q);

    a.p += e_y * (a.s.y - h);
    a.s.y = h;

    return a;
}

Quad shrunken(Quad a, float inset)
{
    v3 e_x = rotate_vector(V3_X, a.q);
    v3 e_y = rotate_vector(V3_Y, a.q);

    a.p += e_x * inset;
    a.p += e_y * inset;

    a.s.x -= inset * 2;
    a.s.y -= inset * 2;

    return a;
}



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
    int revolutions = angle / (double)TAU;
    angle -= revolutions * (double)TAU;
    return angle;
}

template<typename T>
T clamp(T value, T min = 0, T max = 1)
{
    if(value < min) return min;
    if(value > max) return max;
    return value;
}

template<typename T, typename U, typename V>
T lerp(T x, U y, V T)
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
bool circle_contains_point(v2 p, v2 center, float radius)
{
    return (magnitude(p - center) <= radius);
}


float area_of_triangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
    return fabs((x1*(y2-y3) + x2*(y3-y1) + x3*(y1-y2))/2.0f);
}

//NOTE: A, B and C are the triangle's vertices. P is the point.
bool triangle_contains_point(v2 P, v2 A, v2 B, v2 C)
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

bool triangle_contains_point(float Px, float Py,
                             float Ax, float Ay, float Bx, float By, float Cx, float Cy)
{
    return triangle_contains_point({ Px, Py }, { Ax, Ay },
                                   { Bx, By }, { Cx, Cy });
}


//NOTE: a is in radians
Quat axis_rotation(v3 axis, float a)
{
    return { sinf(a/2.0f) * axis.x,
             sinf(a/2.0f) * axis.y,
             sinf(a/2.0f) * axis.z,
             cosf(a/2.0f) };
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
    return { q.x, q.y, q.z, -q.w };
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





float point_ray_distance(v3 point, Ray ray)
{
    float t = dot(point-ray.p0, ray.dir);
    v3    p = ray.p0 + t * ray.dir;
    return magnitude(p - point);
}

bool ray_intersects_plane(Ray ray, v4 plane, v3 *_p, float *_t = NULL)
{
    if(is_zero(dot(ray.dir, plane.xyz))) return false;

    float a = plane.x;
    float b = plane.y;
    float c = plane.z;
    float d = plane.w;

    float x0 = ray.p0.x;
    float y0 = ray.p0.y;
    float z0 = ray.p0.z;

    float dx = ray.dir.x;
    float dy = ray.dir.y;
    float dz = ray.dir.z;
    
    float t = (-d - a*x0 - b*y0 - c*z0) / (a*dx + b*dy + c*dz);
    *_p = ray.p0 + ray.dir * t;

    if(_t) *_t = t;
      
    return true;
}


v4 triangle_plane(v3 a, v3 b, v3 c)
{
    v4 plane;
    plane.xyz = normalize(cross((b - a), (c - a)));
    plane.w   = -dot(a, plane.xyz);
    return plane;
}



v3 barycentric(v3 p, v3 a, v3 b, v3 c)
{
    v3 result;
    
    v3 v0 = b - a;
    v3 v1 = c - a;
    v3 v2 = p - a;
        
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    
    float denom = d00 * d11 - d01 * d01;
    
    result.y = (d11 * d20 - d01 * d21) / denom;
    result.z = (d00 * d21 - d01 * d20) / denom;
    result.x = 1.0f - result.y - result.z;

    return result;
}


/*
  NOTE: This function could be dramatically faster!
        Especially if we know that the ray won't come from "behind".
 */

bool ray_intersects_aabb(Ray ray, AABB bbox, v3 *_intersection = NULL, float *_ray_t = NULL)
{
    v3 bbox_p1 = bbox.p + bbox.s;

    v3 center = bbox.p + bbox.s * 0.5f;
    float max_radius = magnitude(bbox_p1 - center); // Distance to a corner from center
    
    // Sphere
    if(point_ray_distance(center, ray) > max_radius) return false;
    
    float x0 = bbox.p.x;
    float y0 = bbox.p.y;
    float z0 = bbox.p.z;

    float x1 = bbox_p1.x;
    float y1 = bbox_p1.y;
    float z1 = bbox_p1.z;

    bool any_hit = false;
    float closest_hit_t = FLT_MAX;
    v3 closest_hit;

    v3 quad_a = bbox.p;
    
    // Sides
    for(int i = 0; i < 6; i++)
    {
        if(i == 3) {
            quad_a = bbox_p1;

            float dx = (x1 - x0); 
            float dy = (y1 - y0); 
            float dz = (z1 - z0);

            x0 = x1;
            y0 = y1;
            z0 = z1;
            
            x1 = x0 - dx;
            y1 = y0 - dy;
            z1 = z0 - dz;
        }
        
        v3 quad_b, quad_c, quad_d;

        switch(i % 3) {
            case 0:
                quad_b.x = x1;
                quad_b.y = y0;
                quad_b.z = z0;

                quad_c.x = x0;
                quad_c.y = y0;
                quad_c.z = z1;
                
                quad_d.x = x1;
                quad_d.y = y0;
                quad_d.z = z1;
                break;

            case 1:
                quad_b.x = x0;
                quad_b.y = y1;
                quad_b.z = z0;

                quad_c.x = x0;
                quad_c.y = y0;
                quad_c.z = z1;
                
                quad_d.x = x0;
                quad_d.y = y1;
                quad_d.z = z1;
                break;

            case 2:
                quad_b.x = x1;
                quad_b.y = y0;
                quad_b.z = z0;

                quad_c.x = x0;
                quad_c.y = y1;
                quad_c.z = z0;
                
                quad_d.x = x1;
                quad_d.y = y1;
                quad_d.z = z0;
                break;
        }

        v4 plane = triangle_plane(quad_a, quad_b, quad_c);

        // Plane
        v3 intersection;
        float ray_t;
        if(!ray_intersects_plane(ray, plane, &intersection, &ray_t)) continue;
        if(any_hit && ray_t >= closest_hit_t) continue;

        
        bool hit = false;
        
        // Triangles
        v3 b;

        b = barycentric(intersection, quad_a, quad_b, quad_c);
        if(b.x >= 0 && b.y >= 0 && b.z >= 0) hit = true;
        
        b = barycentric(intersection, quad_b, quad_c, quad_d);
        if(b.x >= 0 && b.y >= 0 && b.z >= 0) hit = true;

        if(!hit) continue;
        

        any_hit = true;
        closest_hit   = intersection;
        closest_hit_t = ray_t;
    }

    if(!any_hit) return false;
    
    if(_intersection) *_intersection = closest_hit;
    if(_ray_t)        *_ray_t        = closest_hit_t;
    return true;
}

bool aabb_intersects_aabb(AABB a, AABB b)
{
    // @Speed: Do we want to represent AABBs as centerpoint, "radiuses" instead, so we don't need to calculate that here?
    
    auto a_r = a.s * 0.5f;
    auto a_c = a.p + a_r;
    
    auto b_r = b.s * 0.5f;
    auto b_c = b.p + b_r;

    if (fabs(a_c.x - b_c.x) >= (a_r.x + b_r.x)) return false;
    if (fabs(a_c.y - b_c.y) >= (a_r.y + b_r.y)) return false;
    if (fabs(a_c.z - b_c.z) >= (a_r.z + b_r.z)) return false;

    return true;
}

bool aabb_contains_point(AABB bbox, v3 p)
{
    v3 bbox_p1 = bbox.p + bbox.s;
    return (p.x >= bbox.p.x && p.y >= bbox.p.y && p.z >= bbox.p.z &&
            p.x < bbox_p1.x && p.y < bbox_p1.y && p.z < bbox_p1.z);
       
}
