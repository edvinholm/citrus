
struct Text_Location
{
    strlength byte; // Where in the string, in bytes
    int cp_index;   // Codepoint index in the string
    int line;       // Line index
    int col;        // Column index, in codepoints
};

struct Body_Text_Line
{
    strlength start_byte;
    int start_cp;
};

struct Body_Text
{
    String text; // Not owned.
    Array<Body_Text_Line, ALLOC_TMP> lines;
    int num_codepoints;

    float start_x;
    float w;
    v2 end_p;
    int end_cp;

    Font_ID font;
    Font_Size font_size;

    float ascent;
    float descent;
    float line_height;
    
    float glyph_scale;
};

Body_Text create_body_text(String text, Rect a, Font_Size font_size, Font_ID font_id, Font_Table *fonts, float start_x = 0, bool multiline = true);
Text_Location text_location_from_position(v2 p, Body_Text *bt, v2 bt_p, Font_Table *fonts);

int line_from_codepoint_index(int cp_index, Body_Text *bt, int *_cp_index_on_line = NULL);

float x_from_codepoint_index(int cp_index_to_find, u8 *start, u8 *end, Body_Text *bt, Font_Table *fonts, int *_cp = NULL);
int codepoint_index_from_x(float x, u8 *start, u8 *end, Body_Text *bt, Font_Table *fonts, bool place_before_trailing_newline, strlength *_rel_byte = NULL);
