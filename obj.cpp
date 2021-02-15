

void skip_whitespace(u8 **at, u8 *end)
{
    while(*at < end) {
        u8 ch = **at;
        if(ch == ' ' || ch == '\n' || ch == '\r')  {
            (*at)++;
            continue;
        }
        break;
    }
}

void skip_line(u8 **at, u8 *end)
{
    while(*at < end) {
        u8 ch = **at;
        (*at)++;
        if(ch == '\n' || ch == '\r') break;
    }
}

bool eat_token(u8 **at, u8 *end, String *_token)
{
    skip_whitespace(at, end);

    Assert(*at <= end);
    if(*at >= end) return false;

    String token = {0};
    token.data = *at;
    
    while(*at < end)
    {
        u8 ch = **at;
        if(ch == ' ') break;
        if(ch == '#' || ch == '/' || ch == '\n')
        {
            if(*at == token.data)
                (*at)++;
                
            break;
        }
        (*at)++;
    }

    token.length = (*at - token.data);
    *_token = token;

    return true;
}


bool read_obj(String contents)
{
    u8 *at  = contents.data;
    u8 *end = contents.data + contents.length;
   
    while(at < end) {

        String token;
        if(!eat_token(&at, end, &token)) break;

        if(equal(token, "#")) {
            skip_line(&at, end);
        }
        else if(equal(token, "mtllib")) {
            // Ignore this.
            skip_line(&at, end);
        }
        else if(equal(token, "g")) {
            // @Norelease @Incomplete
            skip_line(&at, end);
        }
        else if(equal(token, "v")) {
            // @Norelease @Incomplete
            skip_line(&at, end);
        }
        else if(equal(token, "vn")) {
            // @Norelease @Incomplete
            skip_line(&at, end);
        }
        else if(equal(token, "vt")) {
            // @Norelease @Incomplete
            skip_line(&at, end);
        }
        else if(equal(token, "usemtl")) {
            // @Norelease @Incomplete
            skip_line(&at, end);
        }
        else if(equal(token, "f")) {
            // @Norelease @Incomplete
            skip_line(&at, end);
        }
        else {
            Debug_Print("Unknown beginning of line: '%.*s'.\n", (int)token.length, token.data);
        }
    }

    return true;
}
