
// @Jai: Make this a "typed" struct initializer instead of calling string().
#define STRING(cstring) string(cstring, sizeof(cstring)-1)

// This is for printf("...%.*s...", HERE)
#define strprint(string) string.length, string.data

typedef s64 strlength; //IMPORTANT: If we change this, we should also change STRING_LENGTH_MAX.
#define STRING_LENGTH_MAX S64_MAX

struct String
{
    u8 *data;
    strlength length;

    u8 operator [](int index);
};


void clear(String *str, Allocator_ID allocator);
void clear_deep(String *str, Allocator_ID allocator);

const String EMPTY_STRING = {NULL, 0};


inline
u8 String::operator [](int index)
{
    return this->data[index];
}


String copy_cstring_to_string(u8 *cstring, strlength Length = -1);

String allocate_string(strlength length, Allocator_ID allocator);

String copy_string(String string);

String concat(String string1, String string2);
String concat(String *strings, int num_strings, bool release_sub_strings = false);

String string_from_double(double dbl);
String string_from_long(u32 integer);

bool starts_with(String s, u8 *start);
bool starts_with(String s, String start);
String sub_string(String s, strlength start, strlength end, Allocator_ID allocator);

//IMPORTANT: F means the original string will be freed.
void sub_string_f(String *original, String *output, strlength start, strlength end);

String insert_character(String s, u8 character, int index);

bool equal(String string1, String string2);


void free_string(String &string);


void print(String s);
