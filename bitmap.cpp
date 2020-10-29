

Pixel *allocate_bitmap(u32 w, u32 h, Allocator_ID allocator)
{
    Pixel *pixels = (Pixel *)alloc(w * h * sizeof(Pixel), allocator);
    Assert(pixels);

    return pixels;
}

void write_pixels_to_bitmap(Pixel *pixels, u32 pixels_w, u32 pixels_h,
                            Pixel *bitmap, u32 bitmap_w, u32 bitmap_h,
                            u32 origin_x, u32 origin_y)
{
    Assert(bitmap_w >= origin_x + pixels_w);
    Assert(bitmap_h >= origin_y + pixels_h);

    Pixel *dst = bitmap + origin_y * bitmap_w + origin_x;
    Pixel *src = pixels;
    for(u32 y = 0; y < pixels_h; y++)
    {
        memcpy(dst, src, sizeof(Pixel) * pixels_w);
        
        dst += bitmap_w;
        src += pixels_w;
    }
}

