


float random_float()
{
    return ((float)rand()) / RAND_MAX;
}

int random_int(int min, int max)
{
    float delta = max - min;
    return round(min + random_float() * delta);
}
