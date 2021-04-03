


float random_float()
{
    return ((float)rand()) / RAND_MAX;
}

int random_int(int min, int max)
{
    float delta = max - min;
    return round(min + random_float() * delta);
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
