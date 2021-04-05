


float ease_out_elastic(float t)
{
    const float tau_over_three = TAU / 3.0;

    t = clamp(t);
    
    return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * tau_over_three) + 1;
}

float ease_out_quint(float t)
{
    return 1.0f - powf(1.0f - t, 5.0f);
}
