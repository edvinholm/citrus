

// FROM: https://alienryderflex.com/saturation.html
//       (Modified a little)

//  public-domain function by Darel Rex Finley
//
//  The passed-in RGB values can be on any desired scale, such as 0 to
//  to 1, or 0 to 255.  (But use the same scale for all three!)
//
//  The "factor" parameter works like this:
//    0.0 creates a black-and-white image.
//    0.5 reduces the color saturation by half.
//    1.0 causes no change.
//    2.0 doubles the color saturation.
//  Note:  A "factor" value greater than 1.0 may project your RGB values
//  beyond their normal range, in which case you probably should truncate
//  them to the desired range before trying to use them in an image.
void adjust_saturation(v4 *color, float factor)
{
    const float Pr = .299;
    const float Pg = .587;
    const float Pb = .114;
    
    float P = sqrt(Pr * color->r * color->r +
                   Pg * color->g * color->g +
                   Pb * color->b * color->b);

    color->r = P + (color->r - P) * factor;
    color->g = P + (color->g - P) * factor;
    color->b = P + (color->b - P) * factor;
}


void adjust_brightness(v4 *color, float factor)
{
    color->rgb *= factor;
}

v4 adjusted_brightness(v4 color, float factor)
{
    v4 result = color;
    adjust_brightness(&result, factor);
    return result;
}


float saturation_of(v4 color)
{
    float c_min = min(min(color.r, color.g), color.b);
    float c_max = max(max(color.r, color.g), color.b);

    float delta = c_max - c_min;
    if(delta <= 0) return 0;
    
    return delta / c_max;
}


// NOTE: If current saturation is zero, we can't change it -- because we don't know which component(s) to increase and which to decrease.
void set_saturation(v4 *color, float saturation)
{
    float old_saturation = saturation_of(*color);

    if(old_saturation <= 0) return;
    
    adjust_saturation(color, saturation / old_saturation);
}


