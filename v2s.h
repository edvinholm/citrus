

struct v2s
{
    union
    {
        struct
        {
            union
            {
                s32 x;
                s32 a;
                s32 w;
            };
            union
            {
                s32 y;
                s32 b;
                s32 h;
                s32 z;
            };
        };
        s32 comp[2];
    };

};
