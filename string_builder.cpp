


//IMPORTANT: :StringBuilderResetResponsability
//           You are responsible for resetting the builder
//           when you are done with it - you should not need
//           to reset it before start appending stuff.

void reset(String_Builder &builder)
{
    builder.length = 0;
    
    builder.buffer = NULL;
    builder.capacity = 0;
}

void ensure_string_builder_buffer(String_Builder &builder)
{
    if(builder.buffer == NULL)
    {
        Assert(builder.capacity == 0);
        builder.capacity = STRING_BUILDER_INITIAL_BUFFER_SIZE;
        
        builder.buffer = tmp_alloc(builder.capacity);
    }
}

void string_append(u8 *data, strlength length, String_Builder &builder)
{
    ensure_string_builder_buffer(builder);

    Assert(data + length < builder.buffer ||
           data >= builder.buffer + builder.capacity);
    
    if(length <= 0) return;

    strlength new_length = builder.length + length;

    strlength new_capacity = builder.capacity;
    if(new_capacity == 0) new_capacity = 1;
    while(new_capacity < new_length)
        new_capacity *= 2;

    if(new_capacity != builder.capacity)
    {
        u8 *new_buffer = tmp_realloc(builder.buffer, builder.capacity, new_capacity);
        memcpy(new_buffer, builder.buffer, builder.length);
        
        builder.buffer = new_buffer;
        builder.capacity = new_capacity;
    }

    memcpy(builder.buffer + builder.length, data, length);
    builder.length = new_length;
}

inline
void string_append(String string, String_Builder &builder)
{
    string_append(string.data, string.length, builder);
}

//TODO @Speed: We make a new string and check the length of it and stuff for each character :(
inline
void string_append_char(char character, String_Builder &builder)
{
    String char_string = {(u8 *)&character, 1};
    string_append(char_string, builder);
}

inline
void string_append(char *cstring, String_Builder &Builder)
{
    string_append(string(cstring), Builder);
}

inline
void string_append(const char *cstring, String_Builder &Builder)
{
    string_append(string((char *)cstring), Builder);
}


inline
void string_append(String String, char *cstring, String_Builder &Builder)
{
    string_append(String, Builder);
    string_append(cstring, Builder);
}


inline
void string_append(const char *cstring, String String, String_Builder &Builder)
{
    string_append(cstring, Builder);
    string_append(String, Builder);
}

inline
void string_append(const char *cstring1, const char *cstring2, String_Builder &Builder)
{
    string_append(cstring1, Builder);
    string_append(cstring2, Builder);
}

void string_append_u(u64 integer, String_Builder &builder)
{
    static u8 buffer[22];
    memset(buffer, 0, sizeof(buffer));
    
    u64 Divider = 1;
    while (Divider <= integer)
    {
        Divider *= 10;
    }
    Divider /= 10;
    
    u16 Digits = 0;
    while (true)
    {
        if (Divider)
        {
            buffer[Digits] = '0' + integer / Divider;
            integer %= Divider;
            Divider /= 10;
        }
        else
        {
            buffer[Digits] = '0';
        }
        Digits++;
        if (Divider < 1) break;
    }
    buffer[Digits] = 0;
    
    string_append((char *)buffer, builder);
}

inline
void string_append(s64 integer, String_Builder &builder)
{
    if(integer < 0)
    {
        string_append_char('-', builder);
        integer = -integer;
    }
    string_append_u((u64)integer, builder);
}
    
inline
void string_append_float(float f, String_Builder &builder, int num_decimals = 2)
{
    if(f < 0) {
        string_append_char('-', builder);
        f *= -1;
    }
    
    s64 x = (int)f;
    string_append(x, builder);
    string_append(".", builder);

    s64 dec = (s64)round((f-x)*pow(10.0f, (float)num_decimals));
    for(int d = num_decimals-1; d >= 1; d--)
    {
        if(dec < pow(10.0f, (float)d))
        {
            string_append((s64)0, builder);
        }
    }
    
    string_append(dec, builder);
}


//@JAI
template<typename T, typename U>
void string_append(T a, U b, String_Builder &builder)
{
    string_append(a, builder);
    string_append(b, builder);
}

//@JAI
template<typename T, typename U, typename V>
void string_append(T a, U b, V c, String_Builder &builder)
{
    string_append(a, builder);
    string_append(b, builder);
    string_append(c, builder);
}

//@JAI
template<typename T, typename U, typename V, typename W>
void string_append(T a, U b, V c, W d, String_Builder &builder)
{
    string_append(a, builder);
    string_append(b, builder);
    string_append(c, builder);
    string_append(d, builder);
}

template<typename T, typename U, typename V, typename W, typename X>
void string_append(T a, U b, V c, W d, X e, String_Builder &builder)
{
    string_append(a, builder);
    string_append(b, builder);
    string_append(c, builder);
    string_append(d, builder);
    string_append(e, builder);
}


template<typename T, typename U, typename V, typename W, typename X, typename Y>
void string_append(T a, U b, V c, W d, X e, Y f, String_Builder &builder)
{
    string_append(a, builder);
    string_append(b, builder);
    string_append(c, builder);
    string_append(d, builder);
    string_append(e, builder);
    string_append(f, builder);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z>
void string_append(T a, U b, V c, W d, X e, Y f, Z g, String_Builder &builder)
{
    string_append(a, builder);
    string_append(b, builder);
    string_append(c, builder);
    string_append(d, builder);
    string_append(e, builder);
    string_append(f, builder);
    string_append(g, builder);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z, typename A>
void string_append(T a, U b, V c, W d, X e, Y f, Z g, A h, String_Builder &builder)
{
    string_append(a, builder);
    string_append(b, builder);
    string_append(c, builder);
    string_append(d, builder);
    string_append(e, builder);
    string_append(f, builder);
    string_append(g, builder);
    string_append(h, builder);
}



//IMPORTANT: This also resets the string builder.
char *copy_built_string_as_cstring(String_Builder &builder, Allocator_ID allocator)
{
    Assert(builder.buffer != NULL);
    
    char *result = (char *)alloc(builder.length+1, allocator);
    memcpy(result, builder.buffer, builder.length);
    result[builder.length] = 0;

    reset(builder);
    return result;
}


//IMPORTANT: This also resets the string builder.
//IMPORTANT: String will not be zero-terminated.
String copy_built_string(String_Builder &builder, Allocator_ID allocator)
{
    Assert(builder.buffer != NULL);
    
    String result;
    if (builder.length)
    {
        result.data = (u8 *)alloc(builder.length, allocator);
        memcpy(result.data, builder.buffer, builder.length);
    }
    else result.data = NULL;
    result.length = builder.length;

    reset(builder);
    return result;
}

//IMPORTANT: This also resets the builder.
String built_string(String_Builder &builder)
{
    String result;
    if(builder.buffer == NULL) result = EMPTY_STRING;
    else {
        result.data = builder.buffer;
        result.length = builder.length;
    }
    
    reset(builder);
    return result;
}




//@JAI
template<typename T, typename U>
String concat_tmp(T a, U b, String_Builder &builder)
{
    string_append(a, b, builder);
    return built_string(builder);
}

//@JAI
template<typename T, typename U, typename V>
String concat_tmp(T a, U b, V c, String_Builder &builder)
{
    string_append(a, b, c, builder);
    return built_string(builder);
}

//@JAI
template<typename T, typename U, typename V, typename W>
String concat_tmp(T a, U b, V c, W d, String_Builder &builder)
{
    string_append(a, b, c, d, builder);
    return built_string(builder);
}

template<typename T, typename U, typename V, typename W, typename X>
String concat_tmp(T a, U b, V c, W d, X e, String_Builder &builder)
{
    string_append(a, b, c, d, e, builder);
    return built_string(builder);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y>
String concat_tmp(T a, U b, V c, W d, X e, Y f, String_Builder &builder)
{
    string_append(a, b, c, d, e, f, builder);
    return built_string(builder);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z>
String concat_tmp(T a, U b, V c, W d, X e, Y f, Z g, String_Builder &builder)
{
    string_append(a, b, c, d, e, f, g, builder);
    return built_string(builder);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z, typename A>
String concat_tmp(T a, U b, V c, W d, X e, Y f, Z g, A h, String_Builder &builder)
{
    string_append(a, b, c, d, e, f, g, h, builder);
    return built_string(builder);
}






//@JAI
template<typename T, typename U>
String concat_new(T a, U b, String_Builder &builder, Allocator_ID allocator)
{   
    string_append(a, b, builder);
    return copy_built_string(builder, allocator);
}

//@JAI
template<typename T, typename U, typename V>
String concat_new(T a, U b, V c, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, builder);
    return copy_built_string(builder, allocator);
}

//@JAI
template<typename T, typename U, typename V, typename W>
String concat_new(T a, U b, V c, W d, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, d, builder);
    return copy_built_string(builder, allocator);
}

template<typename T, typename U, typename V, typename W, typename X>
String concat_new(T a, U b, V c, W d, X e, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, d, e, builder);
    return copy_built_string(builder, allocator);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y>
String concat_new(T a, U b, V c, W d, X e, Y f, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, d, e, f, builder);
    return copy_built_string(builder, allocator);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z>
String concat_new(T a, U b, V c, W d, X e, Y f, Z g, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, d, e, f, g, builder);
    return copy_built_string(builder, allocator);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z, typename A>
String concat_new(T a, U b, V c, W d, X e, Y f, Z g, A h, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, d, e, f, g, h, builder);
    return copy_built_string(builder, allocator);
}







//@JAI
template<typename T, typename U>
char *concat_cstring_new(T a, U b, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, builder);
    return copy_built_string_as_cstring(builder, allocator);
}

//@JAI
template<typename T, typename U, typename V>
char *concat_cstring_new(T a, U b, V c, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, builder);
    return copy_built_string_as_cstring(builder, allocator);
}

//@JAI
template<typename T, typename U, typename V, typename W>
char *concat_cstring_new(T a, U b, V c, W d, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, d, builder);
    return copy_built_string_as_cstring(builder, allocator);
}

template<typename T, typename U, typename V, typename W, typename X>
char *concat_cstring_new(T a, U b, V c, W d, X e, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, d, e, builder);
    return copy_built_string_as_cstring(builder, allocator);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y>
char *concat_cstring_new(T a, U b, V c, W d, X e, Y f, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, d, e, f, builder);
    return copy_built_string_as_cstring(builder, allocator);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z>
char *concat_cstring_new(T a, U b, V c, W d, X e, Y f, Z g, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, d, e, f, g, builder);
    return copy_built_string_as_cstring(builder, allocator);
}

template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z, typename A>
char *concat_cstring_new(T a, U b, V c, W d, X e, Y f, Z g, A h, String_Builder &builder, Allocator_ID allocator)
{
    string_append(a, b, c, d, e, f, g, h, builder);
    return copy_built_string_as_cstring(builder, allocator);
}



//@JAI
template<typename T, typename U>
char *concat_cstring_tmp(T a, U b, String_Builder &builder)
{
    string_append(a, b, builder);
    string_append_char(0, builder);
    return (char *)built_string(builder).data;
}

//@JAI
template<typename T, typename U, typename V>
char *concat_cstring_tmp(T a, U b, V c, String_Builder &builder)
{
    string_append(a, b, c, builder);
    string_append_char(0, builder);
    return (char *)built_string(builder).data;
}

//@JAI
template<typename T, typename U, typename V, typename W>
char *concat_cstring_tmp(T a, U b, V c, W d, String_Builder &builder)
{
    string_append(a, b, c, d, builder);
    string_append_char(0, builder);
    return (char *)built_string(builder).data;
}

template<typename T, typename U, typename V, typename W, typename X>
char *concat_cstring_tmp(T a, U b, V c, W d, X e, String_Builder &builder)
{
    string_append(a, b, c, d, e, builder);
    string_append_char(0, builder);
    return (char *)built_string(builder).data;
}

template<typename T, typename U, typename V, typename W, typename X, typename Y>
char *concat_cstring_tmp(T a, U b, V c, W d, X e, Y f, String_Builder &builder)
{
    string_append(a, b, c, d, e, f, builder);
    string_append_char(0, builder);
    return (char *)built_string(builder).data;
}

template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z>
char *concat_cstring_tmp(T a, U b, V c, W d, X e, Y f, Z g, String_Builder &builder)
{
    string_append(a, b, c, d, e, f, g, builder);
    string_append_char(0, builder);
    return (char *)built_string(builder).data;
}

template<typename T, typename U, typename V, typename W, typename X, typename Y, typename Z, typename A>
char *concat_cstring_tmp(T a, U b, V c, W d, X e, Y f, Z g, A h, String_Builder &builder)
{
    string_append(a, b, c, d, e, f, g, h, builder);
    string_append_char(0, builder);
    return (char *)built_string(builder).data;
}







void clear(String_Builder *builder)
{
    /*
    if(builder->buffer)
        tmp_dealloc(builder->buffer);
    */
    Zero(*builder);
}


