


float random_float()
{
    return ((float)rand()) / RAND_MAX;
}

// @Speed!
s64 random_int(s64 min, s64 max){
    double delta = max - min;
    return round(min + (double)random_float() * delta);
}

v3 random_point_in_unit_sphere()
{
    float r     = random_float();
    float theta = random_float() * TAU;
    float phi   = random_float() * TAU;

    return { r * cosf(phi) * sinf(theta),
             r * sinf(phi) * sinf(theta),
             r * cosf(theta)
            };
}
