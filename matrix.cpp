
const m4x4 M_IDENTITY = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
};

#if !(OS_ANDROID) || DEBUG
void print_matrix(float *m, int i, int j)
{
    printf("*------------------------*\n");
    
    for(int r = 0; r < i; r++)
    {
        printf("|");
        for(int c = 0; c < j; c++)
        {
            float f = m[r*4 + c];
            if(f < 0 || (1.0f / f) <= 0)
            { printf("%.2f ", f); }
            else
            { printf(" %.2f ", f); }
        }
        printf("|\n");
    }
    
    printf("*------------------------*\n");
}

inline
void print(m4x4 m)
{
    print_matrix(m.elements, 4, 4);
}
#endif



#if 0
float det(float *a, int k)
{
    
    float s = 1;
    float det_ = 0;
    
    int b_k = k - 1;
    float b[4*4]; //Space for b_k <= 4, but will be used for smaller matrices as well.
    
    det_ = 0;
    if (k == 1)
    {
        return (a[0]);
    }
    else
    {
        for (int c = 0; c < k; c++)
        {
            int m = 0;
            int n = 0;
            for (int i = 0; i < k; i++)
            {
                for (int j = 0; j < k; j++)
                {
                    if (i != 0 && j != c)
                    {
                        b[m*b_k + n] = a[i*k + j];
                        if (n < (k - 2))
                            n++;
                        else
                        {
                            n = 0;
                            m++;
                        }
                    }
                }
            }
            det_ = det_ + s * (a[0 * k + c] * det(b, k - 1));
            s = -1 * s;
        }
    }
 
    return (det_);
}


inline
float det(m4x4 matrix)
{
    return det(matrix.elements, 4);
}
#endif

m4x4 transposed(m4x4 matrix)
{
    m4x4 r = matrix;

    r._01 = matrix._10;
    r._10 = matrix._01;
    
    r._02 = matrix._20;
    r._20 = matrix._02;
    
    r._03 = matrix._30;    
    r._30 = matrix._03;

    r._12 = matrix._21;
    r._21 = matrix._12;
    
    r._23 = matrix._32;
    r._32 = matrix._23;
    
    r._13 = matrix._31;
    r._31 = matrix._13;

    return r;
}


m4x4 inverse_of(m4x4 matrix)
{
    m4x4 inverse;
    float det;

    float *m   = matrix.elements;
    float *inv = inverse.elements;

    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    if (det == 0) {
        Zero(inverse);
        return inverse;
    }

    det = 1.0 / det;

    inv[0] *= det;
    inv[1] *= det;
    inv[2] *= det;
    inv[3] *= det;
    inv[4] *= det;
    inv[5] *= det;
    inv[6] *= det;
    inv[7] *= det;
    inv[8] *= det;
    inv[9] *= det;
    inv[10] *= det;
    inv[11] *= det;
    inv[12] *= det;
    inv[13] *= det;
    inv[14] *= det;
    inv[15] *= det;
    
    return inverse;
}

#if 0
m4x4 inverted_(m4x4 matrix)
{
    float d = det(matrix);
    
    if(is_zero(d)) {
        //Assert(false);

        //TODO @Investigate: Is this the right thing to do? We just shoot stuff out in space and scale it to zero.
        m4x4 m = {0};
        m._03 = m._13 = m._23 = -99999999999;
        m._33 = 1;
        
        return m;
    }

    //FROM: https://www.sanfoundry.com/c-program-find-inverse-matrix/
    //@Research how this actually work.
    m4x4 inv;
    
    float b[3*3];
    int p, q, m, n, i, j;
    for (q = 0; q < 4; q++)
    {
        for (p = 0; p < 4; p++)
        {
            m = 0;
            n = 0;
            for (i = 0; i < 4; i++)
            {
                for (j = 0; j < 4; j++)
                {
                    if (i != q && j != p)
                    {
                        b[m*3 + n] = matrix.elements[i*4 + j];
                        if (n < (4 - 2))
                            n++;
                        else
                        {
                            n = 0;
                            m++;
                        }
                    }
                }
            }

            inv.elements[q*4 + p] = pow(-1, q + p) * det(b, 4 - 1);
        }
    }
    
    inv = transposed(inv);
    for(int r = 0; r < 4; r++)
        for(int c = 0; c < 4; c++)
            inv.elements[r*4 + c] /= d;

    return inv;
}
#endif


#define m4x4_set(Matrix, Row, Col, Value) Matrix.elements[(Row * 4) + Col] = Value
#define m4x4_get(Matrix, Row, Col) Matrix.elements[(Row * 4) + Col]



m4x4 make_m4x4(float a, float b, float c, float d,
               float e, float f, float g, float h,
               float i, float j, float k, float l,
               float m, float n, float o, float p)
{
    m4x4 result;
    
    result._00 = a;
    result._01 = b;
    result._02 = c;
    result._03 = d;
    
    result._10 = e;
    result._11 = f;
    result._12 = g;
    result._13 = h;
    
    result._20 = i;
    result._21 = j;
    result._22 = k;
    result._23 = l;
    
    result._30 = m;
    result._31 = n;
    result._32 = o;
    result._33 = p;
    
    return result;
}


inline
void vecmatmuls(m4x4 m, v3 *in, v3 *_out, u32 n)
{
    v3 *in_end = in + n;
    //printf("%d\n", n);
    
    while(in < in_end)
    {
        float *y = &in->y;
        float *z = &in->z;
        
        _out->x = m._00 * (*(float*)in) + m._01 * (*y) + m._02 * (*z) + m._03;
        _out->y = m._10 * (*(float*)in) + m._11 * (*y) + m._12 * (*z) + m._13;
        _out->z = m._20 * (*(float*)in) + m._21 * (*y) + m._22 * (*z) + m._23;

        in++;
        _out++;
    }

}

inline
v3 vecmatmul(m4x4 m, v3 v)
{
    v3 result;
    result.x = m4x4_get(m, 0, 0) * v.x + m4x4_get(m, 0, 1) * v.y + m4x4_get(m, 0, 2) * v.z + m4x4_get(m, 0, 3) * 1.0f;
    result.y = m4x4_get(m, 1, 0) * v.x + m4x4_get(m, 1, 1) * v.y + m4x4_get(m, 1, 2) * v.z + m4x4_get(m, 1, 3) * 1.0f;
    result.z = m4x4_get(m, 2, 0) * v.x + m4x4_get(m, 2, 1) * v.y + m4x4_get(m, 2, 2) * v.z + m4x4_get(m, 2, 3) * 1.0f;
    return result;
}


inline
float vecmatmul_z(m4x4 m, v3 v)
{
    return m4x4_get(m, 2, 0) * v.x + m4x4_get(m, 2, 1) * v.y + m4x4_get(m, 2, 2) * v.z + m4x4_get(m, 2, 3) * 1.0f;
}




inline
v4 vecmatmul(m4x4 m, v4 v)
{
    v4 result;
    result.x = m4x4_get(m, 0, 0) * v.x + m4x4_get(m, 0, 1) * v.y + m4x4_get(m, 0, 2) * v.z + m4x4_get(m, 0, 3) * v.w;
    result.y = m4x4_get(m, 1, 0) * v.x + m4x4_get(m, 1, 1) * v.y + m4x4_get(m, 1, 2) * v.z + m4x4_get(m, 1, 3) * v.w;
    result.z = m4x4_get(m, 2, 0) * v.x + m4x4_get(m, 2, 1) * v.y + m4x4_get(m, 2, 2) * v.z + m4x4_get(m, 2, 3) * v.w;
    result.w = m4x4_get(m, 3, 0) * v.x + m4x4_get(m, 3, 1) * v.y + m4x4_get(m, 3, 2) * v.z + m4x4_get(m, 3, 3) * v.w;
    return result;
}


inline
m4x4 matmul(m4x4 n, m4x4 m)
{
    m4x4 result;
    
    m4x4_set(result, 0, 0, m4x4_get(m,0,0) * m4x4_get(n,0,0) + m4x4_get(m,0,1) * m4x4_get(n,1,0) + m4x4_get(m,0,2) * m4x4_get(n,2,0) + m4x4_get(m,0,3) * m4x4_get(n,3,0));
    m4x4_set(result, 1, 0, m4x4_get(m,1,0) * m4x4_get(n,0,0) + m4x4_get(m,1,1) * m4x4_get(n,1,0) + m4x4_get(m,1,2) * m4x4_get(n,2,0) + m4x4_get(m,1,3) * m4x4_get(n,3,0));
    m4x4_set(result, 2, 0, m4x4_get(m,2,0) * m4x4_get(n,0,0) + m4x4_get(m,2,1) * m4x4_get(n,1,0) + m4x4_get(m,2,2) * m4x4_get(n,2,0) + m4x4_get(m,2,3) * m4x4_get(n,3,0));
    m4x4_set(result, 3, 0, m4x4_get(m,3,0) * m4x4_get(n,0,0) + m4x4_get(m,3,1) * m4x4_get(n,1,0) + m4x4_get(m,3,2) * m4x4_get(n,2,0) + m4x4_get(m,3,3) * m4x4_get(n,3,0));
    
    m4x4_set(result, 0, 1, m4x4_get(m,0,0) * m4x4_get(n,0,1) + m4x4_get(m,0,1) * m4x4_get(n,1,1) + m4x4_get(m,0,2) * m4x4_get(n,2,1) + m4x4_get(m,0,3) * m4x4_get(n,3,1));
    m4x4_set(result, 1, 1, m4x4_get(m,1,0) * m4x4_get(n,0,1) + m4x4_get(m,1,1) * m4x4_get(n,1,1) + m4x4_get(m,1,2) * m4x4_get(n,2,1) + m4x4_get(m,1,3) * m4x4_get(n,3,1));
    m4x4_set(result, 2, 1, m4x4_get(m,2,0) * m4x4_get(n,0,1) + m4x4_get(m,2,1) * m4x4_get(n,1,1) + m4x4_get(m,2,2) * m4x4_get(n,2,1) + m4x4_get(m,2,3) * m4x4_get(n,3,1));
    m4x4_set(result, 3, 1, m4x4_get(m,3,0) * m4x4_get(n,0,1) + m4x4_get(m,3,1) * m4x4_get(n,1,1) + m4x4_get(m,3,2) * m4x4_get(n,2,1) + m4x4_get(m,3,3) * m4x4_get(n,3,1));
    
    m4x4_set(result, 0, 2, m4x4_get(m,0,0) * m4x4_get(n,0,2) + m4x4_get(m,0,1) * m4x4_get(n,1,2) + m4x4_get(m,0,2) * m4x4_get(n,2,2) + m4x4_get(m,0,3) * m4x4_get(n,3,2));
    m4x4_set(result, 1, 2, m4x4_get(m,1,0) * m4x4_get(n,0,2) + m4x4_get(m,1,1) * m4x4_get(n,1,2) + m4x4_get(m,1,2) * m4x4_get(n,2,2) + m4x4_get(m,1,3) * m4x4_get(n,3,2));
    m4x4_set(result, 2, 2, m4x4_get(m,2,0) * m4x4_get(n,0,2) + m4x4_get(m,2,1) * m4x4_get(n,1,2) + m4x4_get(m,2,2) * m4x4_get(n,2,2) + m4x4_get(m,2,3) * m4x4_get(n,3,2));
    m4x4_set(result, 3, 2, m4x4_get(m,3,0) * m4x4_get(n,0,2) + m4x4_get(m,3,1) * m4x4_get(n,1,2) + m4x4_get(m,3,2) * m4x4_get(n,2,2) + m4x4_get(m,3,3) * m4x4_get(n,3,2));
    
    m4x4_set(result, 0, 3, m4x4_get(m,0,0) * m4x4_get(n,0,3) + m4x4_get(m,0,1) * m4x4_get(n,1,3) + m4x4_get(m,0,2) * m4x4_get(n,2,3) + m4x4_get(m,0,3) * m4x4_get(n,3,3));
    m4x4_set(result, 1, 3, m4x4_get(m,1,0) * m4x4_get(n,0,3) + m4x4_get(m,1,1) * m4x4_get(n,1,3) + m4x4_get(m,1,2) * m4x4_get(n,2,3) + m4x4_get(m,1,3) * m4x4_get(n,3,3));
    m4x4_set(result, 2, 3, m4x4_get(m,2,0) * m4x4_get(n,0,3) + m4x4_get(m,2,1) * m4x4_get(n,1,3) + m4x4_get(m,2,2) * m4x4_get(n,2,3) + m4x4_get(m,2,3) * m4x4_get(n,3,3));
    m4x4_set(result, 3, 3, m4x4_get(m,3,0) * m4x4_get(n,0,3) + m4x4_get(m,3,1) * m4x4_get(n,1,3) + m4x4_get(m,3,2) * m4x4_get(n,2,3) + m4x4_get(m,3,3) * m4x4_get(n,3,3));

    return result;
}


inline
m4x4 operator * (m4x4 m, m4x4 n)
{
    return matmul(m, n);
};

inline
void operator *= (m4x4 &m, m4x4 n)
{
    m = matmul(n, m);
}


#if 0 // Not sure this is right
inline
bool equal_2d(m4x4 &m, m4x4 &n) {
    return (floats_equal(m._00, n._00) && floats_equal(m._01, n._01) &&
            floats_equal(m._10, n._10) && floats_equal(m._11, n._11) &&
            floats_equal(m._20, n._20) && floats_equal(m._21, n._21) &&
            floats_equal(m._30, n._30) && floats_equal(m._31, n._31));
}
#endif

inline
bool operator == (m4x4 &m, m4x4 &n)
{
    return (floats_equal(m.elements[0],  n.elements[0])  &&
            floats_equal(m.elements[1],  n.elements[1])  &&
            floats_equal(m.elements[2],  n.elements[2])  &&
            floats_equal(m.elements[3],  n.elements[3])  &&
            floats_equal(m.elements[4],  n.elements[4])  &&
            floats_equal(m.elements[5],  n.elements[5])  &&
            floats_equal(m.elements[6],  n.elements[6])  &&
            floats_equal(m.elements[7],  n.elements[7])  &&
            floats_equal(m.elements[8],  n.elements[8])  &&
            floats_equal(m.elements[9],  n.elements[9])  &&
            floats_equal(m.elements[10], n.elements[10]) &&
            floats_equal(m.elements[11], n.elements[11]) &&
            floats_equal(m.elements[12], n.elements[12]) &&
            floats_equal(m.elements[13], n.elements[13]) &&
            floats_equal(m.elements[14], n.elements[14]) &&
            floats_equal(m.elements[15], n.elements[15])); 
}

inline
bool operator != (m4x4 &m, m4x4 &n)
{
    return !(m == n);
}




m4x4 translation_matrix(v3 dp)
{
    m4x4 result;
    
    m4x4_set(result, 0, 0, 1);
    m4x4_set(result, 0, 1, 0);
    m4x4_set(result, 0, 2, 0);
    m4x4_set(result, 0, 3, dp.x);
    
    m4x4_set(result, 1, 0, 0);
    m4x4_set(result, 1, 1, 1);
    m4x4_set(result, 1, 2, 0);
    m4x4_set(result, 1, 3, dp.y);
    
    m4x4_set(result, 2, 0, 0);
    m4x4_set(result, 2, 1, 0);
    m4x4_set(result, 2, 2, 1);
    m4x4_set(result, 2, 3, dp.z);
    
    m4x4_set(result, 3, 0, 0);
    m4x4_set(result, 3, 1, 0);
    m4x4_set(result, 3, 2, 0);
    m4x4_set(result, 3, 3, 1);

    return result;
}

m4x4 scale_matrix(v3 s)
{
    m4x4 result;
    
    m4x4_set(result, 0, 0, s.x);
    m4x4_set(result, 0, 1, 0);
    m4x4_set(result, 0, 2, 0);
    m4x4_set(result, 0, 3, 0);
    
    m4x4_set(result, 1, 0, 0);
    m4x4_set(result, 1, 1, s.y);
    m4x4_set(result, 1, 2, 0);
    m4x4_set(result, 1, 3, 0);
    
    m4x4_set(result, 2, 0, 0);
    m4x4_set(result, 2, 1, 0);
    m4x4_set(result, 2, 2, s.z);
    m4x4_set(result, 2, 3, 0);
    
    m4x4_set(result, 3, 0, 0);
    m4x4_set(result, 3, 1, 0);
    m4x4_set(result, 3, 2, 0);
    m4x4_set(result, 3, 3, 1);

    return result;
}



m4x4 scale_around_point_matrix(v3 scale, v3 point)
{
    //TODO @Speed: Premultiply this into one matrix.
    //TODO @Speed: Premultiply this into one matrix.
    //TODO @Speed: Premultiply this into one matrix.
    //TODO @Speed: Premultiply this into one matrix.
    //TODO @Speed: Premultiply this into one matrix.

    
    m4x4 t1 = translation_matrix(-point);
    m4x4 r = scale_matrix(scale);
    m4x4 t2 = translation_matrix(point);

    return matmul(matmul(t1, r), t2);
}



m4x4 rotation_matrix(Quat q)
{
    //@JAI: Using q;
    
    float x = q.x;
    float y = q.y;
    float z = q.z;
    float w = q.w;
    
    return make_m4x4(
        (1.0f - 2.0f*y*y - 2.0f*z*z), (2.0f*x*y + 2.0f*z*w), (2.0f*x*z - 2.0f*y*w), 0,
        (2.0f*x*y - 2.0f*z*w), (1.0f - 2.0f*x*x - 2.0f*z*z), (2.0f*y*z + 2.0f*x*w), 0,
        (2.0f*x*z + 2.0f*y*w), (2.0f*y*z - 2.0f*x*w), (1.0f - 2.0f*x*x - 2.0f*y*y), 0,
        0,                     0,                     0,                            1);
                     
}




//NOTE: a is in radians
m4x4 rotation_around_point_matrix(Quat rotation, v3 point)
{
    //TODO @Speed: Premultiply this into one matrix.
    //TODO @Speed: Premultiply this into one matrix.
    //TODO @Speed: Premultiply this into one matrix.
    //TODO @Speed: Premultiply this into one matrix.
    //TODO @Speed: Premultiply this into one matrix.

    rotation.xyzw = rotation.xyzw;
    
    m4x4 t1 = translation_matrix(-point);
    m4x4 r = rotation_matrix(rotation);
    m4x4 t2 = translation_matrix(point);

    return matmul(matmul(t1, r), t2);
}



inline
v3 rotate_vector(v3 v, Quat q)
{
    m4x4 m = rotation_matrix(q);
    return vecmatmul(m, v);
}

inline
void rotate_vectors(v3 *vectors, v3 *_output, Quat q, u32 n)
{
    m4x4 matrix = rotation_matrix(q);
    vecmatmuls(matrix, vectors, _output, n);
}

