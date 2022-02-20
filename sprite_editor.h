
enum Sprite_ID;

v4 sprite_editor_background_colors[] = {
    C_BLACK,
    C_WHITE,
    C_GRAY,
    C_RED,
    C_GREEN,
    C_BLUE,
    C_YELLOW
};

struct Sprite_Editor
{
    bool open;

    float scale;
    int background_color_index;

    Sprite_ID selected_sprite;
};
