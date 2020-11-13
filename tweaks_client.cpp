
struct App_Version
{
    u16 comp[4];
};
bool equal(App_Version &a, App_Version &b)
{
    for(int i = 0; i < ARRLEN(a.comp); i++)
        if(a.comp[i] != b.comp[i]) return false;

    return true;
}

App_Version APP_version = {{
#include "_app_version.cpp"
}};


bool TWEAK_vsync_enabled = false;

// Text editing //
float TWEAK_nav_key_repeat_delay = 0.1; // in seconds
// --

const int TWEAK_max_multisample_samples = 8;

const int TWEAK_font_texture_size = 2048;
const float TWEAK_font_oversampling_rate = 2.0f;
