
int ucd_line;
int ucd_col;
char *ucd_file = (char *)"tmp/PropList.txt"; //https://www.unicode.org/Public/UCD/latest/ucd/PropList.txt


#define Error(...) { printf("%s:%d:%d: error: ", ucd_file, ucd_line, ucd_col); printf(__VA_ARGS__); return false; }


enum UCD_Token_Type
{
    UCD_TKN_UNKOWN = 0,
    UCD_TKN_COMMENT_START,
    UCD_TKN_PERIOD,
    UCD_TKN_SEMICOLON,
    UCD_TKN_NEWLINE,

    UCD_TKN_NONE_OR_NUM
};

const char *ucd_token_type_names[UCD_TKN_NONE_OR_NUM] = {
    "UNKNOWN",
    "COMMENT_START",
    "PERIOD",
    "SEMICOLON",
    "NEWLINE"
};

struct UCD_Token
{
    UCD_Token_Type type;
    
    union
    {
        String str;
        struct {
            u8 *data;
            strlength length;
        };
    };
};

enum UCD_Codepoint_Property
{
    UCD_CP_PROP_WHITE_SPACE,
    UCD_CP_PROP_QUOTATION_MARK,
    UCD_CP_PROP_SENTENCE_TERMINAL,

    UCD_CP_PROP_NONE_OR_NUM
};

const char *ucd_codepoint_property_names[UCD_CP_PROP_NONE_OR_NUM] = {
    "White_Space",
    "Quotation_Mark",
    "Sentence_Terminal"
};

struct UCD_Registry
{
    Array<UCD_Codepoint_Interval> intervals_with_property[UCD_CP_PROP_NONE_OR_NUM];
};

void add_ucd_codepoint_interval(UCD_Codepoint_Interval interval, String property_name, UCD_Registry *registry)
{
    UCD_Codepoint_Property property = UCD_CP_PROP_NONE_OR_NUM;
    
    for(int i = 0; i < ARRLEN(ucd_codepoint_property_names); i++)
    {
        if(equal(STRING(ucd_codepoint_property_names[i]), property_name))
        {
            property = (UCD_Codepoint_Property)i;
        }
    }

    if(property == UCD_CP_PROP_NONE_OR_NUM) return;

    array_add(registry->intervals_with_property[property], interval);
}

bool eat_ucd_token(u8 **at, u8 *end, UCD_Token *_token = NULL)
{
    if(*at >= end) return false;
    
    // Skip whitespace
    while(*at < end)
    {
        if(**at != ' ') break;
        (*at)++;
    }

    UCD_Token result;
    Zero(result);
    result.data = *at;

    while(*at < end)
    {
        u8 ch = **at;

        if(ch == '#' || ch == ';' || ch == ' ' || ch == '\n' || ch == '.')
        {
            if(result.length == 0)
            {
                if(ch == '#')  result.type = UCD_TKN_COMMENT_START;
                if(ch == '.') result.type = UCD_TKN_PERIOD;
                if(ch == ';')  result.type = UCD_TKN_SEMICOLON;
                if(ch == '\n') {
                    result.type = UCD_TKN_NEWLINE;
                    ucd_line++;
                    ucd_col = 1;
                }
                
                result.length = 1;
                (*at)++;
            }
                
            break;
        }

        result.length++;
        (*at)++;
    }

    if(_token) *_token = result;
    
    if(result.type != UCD_TKN_NEWLINE)
        ucd_col += result.length;
    
    return true;
}

bool eat_ucd_token_if_type(UCD_Token_Type type, u8 **at, u8 *end)
{
    u8 *old_at = *at;
    
    UCD_Token token;
    if(!eat_ucd_token(at, end, &token)) return false;

    if(token.type == type) return true;

    *at = old_at;
    return false;
}

bool expect_ucd_token(UCD_Token_Type expected_type, u8 **at, u8 *end)
{
    UCD_Token token;
    if(!eat_ucd_token(at, end, &token)) return false;

    if(token.type != expected_type)
    {
        Error("Expected %s token, but found %s (\"%.*s\")\n",
              ucd_token_type_names[expected_type],
              ucd_token_type_names[token.type],
              token.str.length, token.str.data);
    }

    return true;
}

bool parse_ucd_hex_codepoint(u8 **at, u8 *end, int *_cp)
{
    int result = 0;
    
    UCD_Token token;
    if(!eat_ucd_token(at, end, &token)) return false;

    u8 *hex_at = token.data;
    u8 *hex_end = hex_at + token.length;
    while(hex_at < hex_end)
    {
        u8 ch = *hex_at;

        if((ch < '0' || ch > '9') &&
           (ch < 'a' || ch > 'f') &&
           (ch < 'A' || ch > 'F')) return false;

        if(ch >= 'a' && ch <= 'f')
            ch -= ('a' - 'A'); // Make it uppercase

        int dec;
        
        if(ch >= '0' && ch <= '9')
            dec = ch - '0';
        else
            dec = ch - 'A' + 10;

        result = result * 16 + dec;
        
        hex_at++;
    }

    *_cp = result;
    return true;
}

bool parse_ucd_codepoint_interval(u8 **at, u8 *end, UCD_Codepoint_Interval *_interval)
{
    UCD_Codepoint_Interval result = {0};
    
    UCD_Token token;

    if(!parse_ucd_hex_codepoint(at, end, &result.cp0)) Error("Unable to parse hex codepoint.\n");

    if(eat_ucd_token_if_type(UCD_TKN_PERIOD, at, end))
    {
        if(!expect_ucd_token(UCD_TKN_PERIOD, at, end)) return false;

        if(!parse_ucd_hex_codepoint(at, end, &result.cp1)) Error("Unable to parse hex codepoint.\n");
    }
    else
    {
        result.cp1 = result.cp0;
    }

    *_interval = result;
    return true;
}

bool parse_ucd_file(u8 **at, u8 *end, UCD_Registry *registry)
{
    
    UCD_Token token;

    bool in_comment = false;
    while(eat_ucd_token(at, end, &token))
    {
//        printf("%.*s\n", token.length, token.data);
        
        if(token.type == UCD_TKN_COMMENT_START)
        {
            in_comment = true;
        }

        if(token.type == UCD_TKN_NEWLINE)
        {
            in_comment = false;
            continue;
        }
        
        if(in_comment) continue;

        *at = token.data;
        
        UCD_Codepoint_Interval interval;
        if(!parse_ucd_codepoint_interval(at, end, &interval))
        {
            Error("Failed to parse codepoint interval.\n");
        }
        
        if(!expect_ucd_token(UCD_TKN_SEMICOLON, at, end)) return false;

        if(!eat_ucd_token(at, end, &token)) Error("Unable to parse codepoint property.\n");
        String property = token.str;

        add_ucd_codepoint_interval(interval, property, registry);

//        printf("----------- %d -> %d => %.*s\n", interval.cp0, interval.cp1, property.length, property.data);
    }

    return true;
}

void output_ucd_cp_prop_checker_function(FILE *file, char *function_name, UCD_Codepoint_Property property)
{
    char *prop_name = (char *)ucd_codepoint_property_names[property];
    
    fprintf(file, "bool %s(int cp)\n{\n", function_name);
    fprintf(file, "    for(int i = 0; i < ARRLEN(%s_codepoints); i++){\n", prop_name);
    fprintf(file, "        UCD_Codepoint_Interval &interval = %s_codepoints[i];\n", prop_name);
    fprintf(file, "        if(interval.cp0 <= cp && interval.cp1 >= cp) return true;\n");
    fprintf(file, "    }\n");
    fprintf(file, "\n");
    fprintf(file, "    return false;\n");
    fprintf(file, "}\n\n");
}

void output_ucd_registry(UCD_Registry *registry, char *filename)
{
    FILE *file = open_file(filename, true);
    if(!file) {
        printf("Unable to open '%s' for writing.\n", filename);
        return;
    }
    
    fprintf(file, "\n/* IMPORTANT: This file was auto-generated by parsing data from the Unicode Character Database. */\n\n");
    
    for(int i = 0; i < UCD_CP_PROP_NONE_OR_NUM; i++)
    {
        const char *property_name = ucd_codepoint_property_names[i];
        fprintf(file, "\nUCD_Codepoint_Interval %s_codepoints[%d] = {", property_name, registry->intervals_with_property[i].n);

        for(int j = 0; j < registry->intervals_with_property[i].n; j++)
        {
            UCD_Codepoint_Interval interval = registry->intervals_with_property[i][j];
            
            if(j > 0) fprintf(file, ",");
            fprintf(file, "\n    { %d, %d }", interval.cp0, interval.cp1);
        }

        fprintf(file, "\n};\n\n");
    }

    output_ucd_cp_prop_checker_function(file, "is_whitespace", UCD_CP_PROP_WHITE_SPACE);
    output_ucd_cp_prop_checker_function(file, "is_quotation_mark", UCD_CP_PROP_QUOTATION_MARK);
    output_ucd_cp_prop_checker_function(file, "is_sentence_terminal", UCD_CP_PROP_SENTENCE_TERMINAL);
    
    close_file(file);
}

int unicode_db_entry_point(int num_arguments, const char **arguments) {

    printf("\n");

    UCD_Registry registry = {0};
    
    ucd_line = 1;
    ucd_col  = 1;
    
    u8 *proplist_content;
    u32 proplist_content_length;
    if(!read_entire_file(ucd_file, &proplist_content, &proplist_content_length))
    {
        printf("Unable to read proplist file.\n");
        return 1;
    }

    u8 *at = proplist_content;
    u8 *end = at + proplist_content_length;

    if(!parse_ucd_file(&at, end, &registry))
        return 2;
    
    for(int i = 0; i < UCD_CP_PROP_NONE_OR_NUM; i++)
    {
        const char *property_name = ucd_codepoint_property_names[i];
        printf("%s\n", property_name);
        for(int c = 0; c < strlen(property_name); c++)
            printf("-");
        printf("\n");

        for(int j = 0; j < registry.intervals_with_property[i].n; j++)
        {
            UCD_Codepoint_Interval interval = registry.intervals_with_property[i][j];
            printf("%d -> %d\n", interval.cp0, interval.cp1); 
        }

        printf("\n");
    }


    output_ucd_registry(&registry, (char *)"ucd.gen.cpp");

    
    //dealloc(proplist_content);
    return 0;
}
