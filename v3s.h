
struct v3s
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
                        s32 x;
                        s32 r;
                        s32 w;
                    };
                    union
                    {
                        s32 y;
                        s32 g;
                        s32 h;
                    };
                };
                v2s xy;
            };
            union
            {
                s32 z;
                s32 b;
                s32 d;
            };
        };
        s32 comp[3];
    };
};
