
bool is_ascii_newline(u8 ch) {
    return (ch == '\n');
}

bool is_ascii_whitespace(u8 ch) {
    return (ch == ' ' || ch == '\r' || ch == '\t' || is_ascii_newline(ch));
}

void skip_whitespace_ascii(u8 **at, u8 *end, bool skip_newlines)
{
    while(*at < end) {

        u8 ch = **at;

        if(is_ascii_whitespace(ch)) {
           if(is_ascii_newline(ch)) {
               if(!skip_newlines) break;
           }
        }
        else break;

        (*at)++;
    }
}

bool eat_token_ascii(u8 **at, u8 *end, String *_token, bool allow_skipping_newlines = true)
{
    u8 *start;

    skip_whitespace_ascii(at, end, allow_skipping_newlines);

    start = *at;
    while(*at < end) {
        if(is_ascii_whitespace(**at)) break;
        (*at)++;
    }

    strlength length = *at-start;
    if(length == 0) return false;

    _token->data   = start;
    _token->length = length;
    return true;
}

bool skip_to_next_line_ascii(u8 **at, u8 *end)
{
    bool found_newline = false;
    
    while(*at < end) {
        
        if(**at == '\n') {
            found_newline = true;
            (*at)++;
            break;
        }
        
        (*at)++;
    }

    return found_newline;
}

bool parse_bool_ascii(u8 **at, u8 *end, bool *_value)
{
    String token;
    if(!eat_token_ascii(at, end, &token, false)) return false;

    if(equal("true", token)) {
        *_value = true;
        return true;
    }

    if(equal("false", token)) {
        *_value = false;
        return true;
    }

    return false;
}

bool parse_uint_ascii(u8 **at, u8 *end, u32 *_value, bool skip_whitespace)
{
    if(skip_whitespace) {
        skip_whitespace_ascii(at, end, false);
    }

    u32 result = 0;

    int digits_found = 0;
    while(*at < end) {
        if(**at >= '0' && **at <= '9') {

            result *= 10;
            result += **at - '0';
            
            digits_found++;
            (*at)++;
        }
        else break;
    }

    if(digits_found == 0) return false;

    *_value = result;
    return true;
}

bool parse_int_ascii(u8 **at, u8 *end, s32 *_value, bool skip_whitespace, bool allow_negative = true)
{
    bool negative = false;

    if(allow_negative && **at == '-') {
        negative = true;
        (*at)++;
    }
    
    u32 uint;
    if(!parse_uint_ascii(at, end, &uint, skip_whitespace)) return false;

    if(uint & 0x80000000) {
        // Does not fit in s32
        Assert(false);
        return false;
    }

    s32 result = uint;
    if(negative) result *= -1;
    
    *_value = result;
    return true;
}

bool parse_float_ascii(u8 **at, u8 *end, float *_value)
{
    s32 left;
    if(!parse_int_ascii(at, end, &left, true)) {
        left = 0;
        if(**at != '.') return false;
    }

    if(*at == end || **at != '.') {
        *_value = (float)left;
        return true;
    }
    (*at)++;
    
    u32 right;
    if(!parse_uint_ascii(at, end, &right, false)) return false;

    float result = right;
    while(result >= 1.0f) result /= 10.0f;

    result += abs(left);
    if(left < 0) result *= -1;

    *_value = result;
    return true;
}

bool parse_tweaks(u8 *start, u8 *end) {

    bool result = true;

    u8 *at = start;
    String token;

    while(true) {
        skip_whitespace_ascii(&at, end, true);
        
        if(at < end && *at == '#') { // Comment
            skip_to_next_line_ascii(&at, end);
            continue;
        }
        
        if(!eat_token_ascii(&at, end, &token, true)) break;

        Tweak_Info  *info  = NULL;
        Tweak_Value *value = NULL;
        for(int i = 0; i < ARRLEN(tweaks.infos); i++) {
            if(equal(tweaks.infos[i].name, token)) {
                info  = tweaks.infos + i;
                value = tweaks.values + i;
                break;
            }
        }

        if(!info) {
            Debug_Print("Unknown tweak '%.*s'.\n", (int)token.length, token.data);
            result = false;
            skip_to_next_line_ascii(&at, end);
            continue;
        }

        Assert(info);
        Assert(value);

        Assert(info->num_components >= 1);
        Assert(info->num_components <= MAX_TWEAK_VALUE_COMPONENTS);
        
        switch(info->type) {
            case TWEAK_TYPE_BOOL: {
                for(int c = 0; c < info->num_components; c++) {
                    if(!parse_bool_ascii(&at, end, &value->bool_values[c])) {
                        Debug_Print("Unable to parse bool value #%d for '%s' tweak.\n", c, info->name);
                        result = false;
                    }
                }
            } break;
                
            case TWEAK_TYPE_FLOAT: {
                for(int c = 0; c < info->num_components; c++) {
                    if(!parse_float_ascii(&at, end, &value->float_values[c])) {
                        Debug_Print("Unable to parse float value #%d for '%s' tweak.\n", c, info->name);
                        result = false;
                    }
                }
            } break;
                
            case TWEAK_TYPE_INT: {
                for(int c = 0; c < info->num_components; c++) {
                    if(!parse_int_ascii(&at, end, &value->int_values[c], true)) {
                        Debug_Print("Unable to parse int value #%d for '%s' tweak.\n", c, info->name);
                        result = false;
                    }
                }
            } break;

            case TWEAK_TYPE_UINT: {
                for(int c = 0; c < info->num_components; c++) {
                    if(!parse_uint_ascii(&at, end, &value->uint_values[c], true)) {
                        Debug_Print("Unable to parse uint value #%d for '%s' tweak.\n", c, info->name);
                        result = false;
                    }
                }
            } break;
                
            default: {
                Assert(false);
                result = false;
                break;
            }
        }

        skip_whitespace_ascii(&at, end, false);
        if(at < end && *at != '\n' && *at != '#') {
            Debug_Print("Trailing characters after '%s' tweak.\n", info->name);
            result = false;
        }
        
        skip_to_next_line_ascii(&at, end);
    }

    return result;
}

bool load_tweaks(char *filename)
{
    FILE *file = open_file(filename, false);
    if(!file) {
        Debug_Print("Unable to open %s.\n", filename);
        return false;
    }
    defer(close_file(file););

    u8 *contents;
    strlength length;
    if(!read_entire_file(file, &contents, ALLOC_TMP, &length)) {
        Debug_Print("Unable to read %s.\n", filename);
        return false;
    }

    if(!parse_tweaks(contents, contents + length)) {
        Debug_Print("Parsing error in %s.\n", filename);
        return false;
    }
    
    return true;
}


// NOTE: If you pass a dev_user_id with length > 0, sb can not be NULL.
bool load_tweaks(String dev_user_id = EMPTY_STRING) {

    load_tweaks("tweaks\\global.tweaks");

    if(dev_user_id.length != 0) {
        load_tweaks(concat_cstring_tmp("tweaks\\", dev_user_id, ".tweaks"));
    }

    load_tweaks("tweaks\\local.tweaks");

    return true;
}


#if OS_WINDOWS
void setup_tweak_hotloading()
{
    String working_dir = platform_get_working_directory(ALLOC_TMP);
    
    const char *subdir = "\\tweaks\\";
    auto subdir_length = cstring_length(subdir);

    auto path_length = working_dir.length + subdir_length;
    u8 *path = tmp_alloc(path_length + 1);
    memcpy(path, working_dir.data, working_dir.length);
    memcpy(path + working_dir.length, subdir, subdir_length);
    path[path_length] = 0;

    Debug_Print("%s\n", path);

    tweaks.dir_change_notif = FindFirstChangeNotification((LPCSTR)path, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
    if(tweaks.dir_change_notif == INVALID_HANDLE_VALUE) {
        Debug_Print("Unable to setup change notification handle for tweaks directory.\n");
        tweaks.dir_change_notif = 0;
    }
}

// NOTE: If you pass a dev_user_id with length > 0, sb can not be NULL.
void maybe_reload_tweaks(String dev_user_id)
{
    if(tweaks.dir_change_notif == 0) return;
    
    if(WaitForSingleObject(tweaks.dir_change_notif, 0) != WAIT_OBJECT_0) return;

    load_tweaks(dev_user_id);

    if(!FindNextChangeNotification(tweaks.dir_change_notif))
    {
        Debug_Print("Unable to FindNextChangeNotification for tweaks directory.\n");
    }
}
#endif


bool tweak_bool(Tweak_ID id, u8 component = 0)
{
    auto *info = tweaks.infos + id;
    Assert(info->type == TWEAK_TYPE_BOOL);
    Assert(component < info->num_components);
    return tweaks.values[id].bool_values[component];
}


s32 tweak_int(Tweak_ID id)
{
    auto *info = tweaks.infos + id;
    Assert(info->type == TWEAK_TYPE_INT);
    return tweaks.values[id].int_values[0];
}

u32 tweak_uint(Tweak_ID id)
{
    auto *info = tweaks.infos + id;
    Assert(info->type == TWEAK_TYPE_UINT);
    return tweaks.values[id].uint_values[0];
}

float tweak_float(Tweak_ID id)
{
    auto *info = tweaks.infos + id;
    Assert(info->type == TWEAK_TYPE_FLOAT);
    return tweaks.values[id].float_values[0];
}

v2s tweak_v2s(Tweak_ID id) {
    auto *info  = tweaks.infos + id;
    auto *value = tweaks.values + id;
    Assert(info->type == TWEAK_TYPE_INT);
    Assert(info->num_components >= 2);

    v2s result;
    result.comp[0] = value->int_values[0];
    result.comp[1] = value->int_values[1];

    return result;
}

v2u tweak_v2u(Tweak_ID id) {
    auto *info  = tweaks.infos + id;
    auto *value = tweaks.values + id;
    Assert(info->type == TWEAK_TYPE_UINT);
    Assert(info->num_components >= 2);

    v2u result;
    result.comp[0] = value->uint_values[0];
    result.comp[1] = value->uint_values[1];

    return result;
}

v2 tweak_v2(Tweak_ID id) {
    auto *info  = tweaks.infos + id;
    auto *value = tweaks.values + id;
    Assert(info->type == TWEAK_TYPE_FLOAT);
    Assert(info->num_components >= 2);

    v2 result;
    result.comp[0] = value->float_values[0];
    result.comp[1] = value->float_values[1];

    return result;
}

v3s tweak_v3s(Tweak_ID id) {
    auto *info  = tweaks.infos + id;
    auto *value = tweaks.values + id;
    Assert(info->type == TWEAK_TYPE_INT);
    Assert(info->num_components >= 3);

    v3s result;
    result.comp[0] = value->int_values[0];
    result.comp[1] = value->int_values[1];
    result.comp[2] = value->int_values[2];

    return result;
}

#if 0 // We don't have a v3u type yet.
v3u tweak_v3u(Tweak_ID id) {
    auto *info  = tweaks.infos + id;
    auto *value = tweaks.values + id;
    Assert(info->type == TWEAK_TYPE_UINT);
    Assert(info->num_components >= 3);

    v3u result;
    result.comp[0] = value->uint_values[0];
    result.comp[1] = value->uint_values[1];
    result.comp[2] = value->uint_values[2];

    return result;
}
#endif

v3 tweak_v3(Tweak_ID id) {
    auto *info  = tweaks.infos + id;
    auto *value = tweaks.values + id;
    Assert(info->type == TWEAK_TYPE_FLOAT);
    Assert(info->num_components >= 3);

    v3 result;
    result.comp[0] = value->float_values[0];
    result.comp[1] = value->float_values[1];
    result.comp[2] = value->float_values[2];

    return result;
}

v4s tweak_v4s(Tweak_ID id) {
    auto *info  = tweaks.infos + id;
    auto *value = tweaks.values + id;
    Assert(info->type == TWEAK_TYPE_INT);
    Assert(info->num_components >= 4);

    v4s result;
    result.comp[0] = value->int_values[0];
    result.comp[1] = value->int_values[1];
    result.comp[2] = value->int_values[2];
    result.comp[3] = value->int_values[3];

    return result;
}

v4u tweak_v4u(Tweak_ID id) {
    auto *info  = tweaks.infos + id;
    auto *value = tweaks.values + id;
    Assert(info->type == TWEAK_TYPE_UINT);
    Assert(info->num_components >= 4);

    v4u result;
    result.comp[0] = value->uint_values[0];
    result.comp[1] = value->uint_values[1];
    result.comp[2] = value->uint_values[2];
    result.comp[3] = value->uint_values[3];

    return result;
}

v4 tweak_v4(Tweak_ID id) {
    auto *info  = tweaks.infos + id;
    auto *value = tweaks.values + id;
    Assert(info->type == TWEAK_TYPE_FLOAT);
    Assert(info->num_components >= 4);

    v4 result;
    result.comp[0] = value->float_values[0];
    result.comp[1] = value->float_values[1];
    result.comp[2] = value->float_values[2];
    result.comp[3] = value->float_values[3];

    return result;
}
