
enum horizontal_alignment
{
    HALIGN_LEFT,
    HALIGN_RIGHT,
    HALIGN_CENTER
};


enum vertical_alignment
{
    VALIGN_TOP,
    VALIGN_BOTTOM,
    VALIGN_CENTER
};


struct m4x4
{
    union
    {
        float elements[4*4];
        struct
        {
            float _00, _01, _02, _03;
            float _10, _11, _12, _13;
            float _20, _21, _22, _23;
            float _30, _31, _32, _33;
        };
    };
};

bool floats_equal(float f, float g);
