
struct v4s
{
    union
    {
        struct
        {
            union
            {
                s32 x;
                s32 a;
            };
            union
            {
                s32 y;
                s32 b;
            };
            
            union
            {
                s32 w;
                s32 c;
            };
            union
            {
                s32 h;
                s32 d;
            };
        };
        s32 comp[4];
    };
};
