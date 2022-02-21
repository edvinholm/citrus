
inline
strlength cstring_length(u8 *cstring)
{
    strlength result = 0;
    while(*(cstring++) != 0) result++;
    return result;
}


inline
strlength cstring_length(const char *cstring)
{
    return cstring_length((u8 *)cstring); //DUDE WHY IS C++ SO STUPID?
}

inline
char *copy_cstring(char *cstring, Allocator_ID allocator)
{
    auto length = cstring_length(cstring);
    char *new_cstring = (char *)alloc(length + 1, allocator);
    memcpy(new_cstring, cstring, length + 1);
    return new_cstring;
}


String copy_cstring_to_string(char *cstring, strlength length, Allocator_ID allocator)
{
    String result;
    strlength str_length = (length == -1) ? cstring_length(cstring) : length;
    result.data = (u8 *)alloc(sizeof(u8)*(str_length+1), allocator);
    memcpy(result.data, cstring, sizeof(u8)*str_length);
    result.data[str_length] = 0;
    result.length = str_length;
    return result;
}

inline
String copy_cstring_to_string(char *cstring, Allocator_ID allocator)
{
    size_t length = strlen(cstring);
    Assert(length <= S32_MAX);
    return copy_cstring_to_string(cstring, (s32)strlen(cstring), allocator);
}

inline
String string(u8 *data, strlength length)
{
    String result;
    result.data = (u8 *)data;
    result.length = length;
    return result;
}

inline
String string(char *data, strlength length)
{
    return string((u8 *)data, length);
}

inline
String string(const char *data, strlength length)
{
    return string((u8 *)data, length);
}

String string(char *data)
{
    return string(data, cstring_length(data));
}

template<Allocator_ID A>
String string(Array<u8, A> &array)
{
    return { array.e, (strlength)array.n };
}

String allocate_string(strlength length, Allocator_ID allocator)
{
    u8 *data = alloc(length, allocator);
    Assert(data);
    return string((char *)data, length);
}

String s64_to_string(s64 i, Allocator_ID allocator)
{
    strlength length = 0;
    
    bool negative = (i < 0);
    if(negative) length++;

    s64 abs_i = abs(i);
    
    strlength num_digits;
    if(abs_i > 0) num_digits = floor(log10(abs_i)) + 1;
    else          num_digits = 1;

    length += num_digits;

    Assert(length > 0);
    String result = allocate_string(length, allocator);

    if(negative) result.data[0] = '-';

    for(strlength c = result.length-1; c >= result.length - num_digits; c--)
    {
        Assert(c < result.length);
        Assert(c >= 0);
        
        result.data[c] = (abs_i % 10) + '0';
        abs_i /= 10;
    }
    
    return result;
}

// @Robustness: This does not check for overflow.
// NOTE: _is_negative is necessary to know if a minus was found if the number is zero. "-0" or "-" will result in the function returning 0.
s64 string_to_s64(String str, bool *_is_negative)
{
    s64 result = 0;
    
    u8 *at  = str.data;
    u8 *end = str.data + str.length;

    bool found_minus = false;
    while(at < end)
    {
        u8 ch = *at;
        if(ch >= '0' && ch <= '9') {
            result *= 10;
            result += (ch - '0');
        }
        else if(ch == '-' && !found_minus) {
            result *= -1;
            found_minus = true;
        }
        
        at++;
    }

    if(found_minus) {
        result *= -1;
        *_is_negative = true;
    } else {
        *_is_negative = false;
    }
    
    return result;
}


char *copy_as_cstring(String s, strlength length, Allocator_ID allocator)
{
    Assert(s.length >= 0);
    Assert(s.length >= length);
    u8 *result = alloc(length+1, allocator);
    if(s.length) memcpy(result, s.data, sizeof(u8)*length);
    result[length] = 0;
    return (char *)result;
}

char *copy_as_cstring(String s, Allocator_ID allocator)
{
    return copy_as_cstring(s, s.length, allocator);
}


String copy_of(String *str, Allocator_ID allocator)
{
    return string(copy_as_cstring(*str, allocator), str->length);
}

String sub_string(String s, strlength start, strlength end = -1)
{
    if(end < 0) end = s.length;
    
    String result = s;
    result.length = end-start;
    result.data = s.data + start;
    return result;
}


bool cstring_starts_with(u8 *string, u8 *substring)
{
    while(*substring != 0)
    {
        if(*string != *substring) return false;
        string++;
        substring++;
    }
    return true;
}

bool cstring_ends_with(u8 *string, u8 *substring)
{
    auto string_length = cstring_length(string);
    auto substring_length = cstring_length(substring);
    if(string_length < substring_length) return false;
    string += string_length - substring_length;

    while(*string != 0)
    {
        if(*string != *substring) return false;
        string++;
        substring++;
    }
    return true;
}

String insert_character(String s, u8 character, strlength index, Allocator_ID allocator)
{
    u8 *new_data = alloc(s.length + 1, allocator);
    if(index > 0)
    {
        memcpy(new_data, s.data, sizeof(u8) * index);
    }
    new_data[index] = character;
    if(s.length > index)
    {
        memcpy(&new_data[index+1], &s.data[index], sizeof(u8)*(s.length-index));
    }
    
    strlength new_length = s.length + 1;
    
    return string(new_data, new_length);
}

String remove_characters(String s, strlength index, strlength Count, Allocator_ID allocator)
{
    u8 *new_data = alloc(s.length-Count, allocator);
    if(index > 0)
    {
        memcpy(new_data, s.data, sizeof(u8) * index);
    }
    if(s.length > index + Count)
    {
        memcpy(&new_data[index], &s.data[index + Count], sizeof(u8) * (s.length - index - Count));
    }
    return string(new_data, s.length - Count);
}

void advance(String &s, strlength amount)
{
    s.data += amount;
    s.length -= amount;
}


bool equal(String s1, String s2)
{
    if(s1.length != s2.length) return false;
    for(int C = 0; C < s1.length; C++)
        if(s1.data[C] != s2.data[C]) return false;
    return true;
}

bool equal(String a, char *b)
{
    strlength b_length = cstring_length(b);
    if(a.length != b_length) return false;
    for(int c = 0; c < a.length; c++)
    {
        if(a[c] != b[c]) return false;
    }
    return true;
}

bool equal(String a, const char *b) { return equal(a, (char *)b); }; // Stupid.
bool equal(char *a, String b) { return equal(b, a); }
bool equal(const char *a, String b) { return equal((char *)a, b); }


inline
bool equal(u8 *cs1, u8 *cs2)
{
    while(true)
    {
        u8 C1 = *cs1++;
        u8 C2 = *cs2++;
        if(C1 != C2) return false;
        if(C1 == 0) break;
    }
    return true;
}



void print(String s)
{
    //@Robustness: printf takes an int here, and string.length is a strlength
    printf("%.*s", (int)s.length, s.data);
}


bool starts_with(String s, u8 *Start)
{
    int C = 0;
    while(Start[C] != 0 && C < s.length)
    {
        if(Start[C] != s.data[C]) return false;
        C++;
    }
    return Start[C] == 0;
}

bool starts_with(String s, const char *start)
{
    return starts_with(s, (u8 *)start);
}



bool starts_with(String s, String Start)
{
    if(s.length < Start.length) return false;
    
    int C = 0;
    while(C < Start.length)
    {
        if(Start.data[C] != s.data[C]) return false;
        C++;
    }
    return true;
}


inline
bool parse_u32(String s, u32 *_i)
{
    bool AnythingFound = false;
    u32 result = 0;
    int C = 0;
    while(C < s.length && result < U32_MAX)
    {
        u8 character = s.data[C];
        if(character >= '0' && character <= '9')
        {
            result = result*10 + (character-'0');
            AnythingFound = true;
        }
        else if(AnythingFound) break;
        C++;
    }

    *_i = result;
    return AnythingFound;
}

bool is_only_digits(String s)
{
    for(int i = 0; i < s.length; i++) {
        if(s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}


bool first_occurrence(const char *sub_string, String s, strlength *_output)
{
    strlength sub_length = cstring_length(sub_string);
    
    u8 *at = s.data;
    u8 *end = s.data + s.length;
    while(at < end)
    {
        if(*at == sub_string[0])
        {
            u8 *Start = at;
            bool Found = true;
            strlength X = 1;
            while(X < sub_length)
            {
                at++;
                if(sub_string[X] != *at)
                {
                    Found = false;
                    break;
                }
                X++;
            }
            if(Found) {
                *_output = Start - s.data;
                return true;
            }
            else at = Start;
        }
        
        at++;
    }
    
    return false;
}

void replace(u8 Old, u8 New, String s)
{
    u8 *at = s.data;
    u8 *end = s.data + s.length;
    while(at < end)
    {
        if(*at == Old)
            *at = New;
        at++;
    }
}



inline
void float_to_string(float F, String *_output, Allocator_ID allocator, int decimals = 2)
{
    int length = (int)log10(F) + decimals;
    if(_output->length < length)
    {
        dealloc(_output->data, allocator);
        _output->data = alloc(length, allocator);
    }
    _output->length = length;
    sprintf((char *)_output->data, "%.*f", decimals, F);
}

inline
float string_to_float(String S)
{
    float result = 0.0f;
    u8 *at = S.data;
    u8 *end = S.data + S.length;
    while(at < end && *at >= '0' && *at <= '9')
    {
        result *= 10;
        result += *at - '0';
        at++;
    }
    Assert(at <= end);
    if(at == end) return result;
    
    if(*at != '.') return result;
    at++;
    
    float decimals = 0.0f;

    while(at < end && *at >= '0' && *at <= '9')
    {
        decimals += *at - '0';
        decimals /= 10;
        at++;
    }
    
    result += decimals;
    return result;
}



void clear(String *str, Allocator_ID allocator)
{
    if(str->data)
        dealloc(str->data, allocator);
}

void clear_deep(String *str, Allocator_ID allocator)
{
    clear(str, allocator);
}
