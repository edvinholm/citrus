
struct v4u
{
    union
    {
        struct
        {
            union
            {
                u32 x;
                u32 a;
            };
            union
            {
                u32 y;
                u32 b;
            };
            
            union
            {
                u32 w;
                u32 c;
            };
            union
            {
                u32 h;
                u32 d;
            };
        };
        u32 comp[4];
    };
};
