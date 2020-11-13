

bool IsUTF16(String string)
{
    //@Speed
    for(strlength C = 0; C < string.length; C++)
        if(string.data[C] == 0) return true;
    return false;
}

inline
bool is_symbol(int codepoint)
{
    switch(codepoint)
    {
        case '.':  case ',': case ':': case ';':
        case '\\': case '|': case '/': case '_':
        case '-':  case '!': case '?': case '{':
        case '}':  case '(': case ')': case '&':
        case '#':  case '%':
        case '@':  case '"': case '*': case '+':
        case '<':  case '>': 
        case '~':
            return true;
    }

    return false;
}

bool is_whitespace_or_symbol(u32 codepoint)
{
    return is_whitespace(codepoint) || is_symbol(codepoint);
}

u8 *find_codepoint_backwards(u8 *at)
{
    at--;
    
    if(!(*at & 0b10000000)) return at; // One byte char

    u8 length = 1;
    while(length < 4)
    {
        if((*at & 0b11000000) == 0b11000000)
        {
            // Found start of cp.
            return at;
        }
        
        length++;
        at--;
    }

    Assert(false);
    return NULL;
}


u32 eat_codepoint(u8 **Pointer, bool UTF16 = false)
{
    if(UTF16)
    {
        u16 Result = (u16)(**Pointer);
        (*Pointer) += sizeof(u16);
        return (u32)Result;
    }
    
    u32 Result = 0;

    u8 NBytes = 0;
    u8 Mask  = 0b10000000;
    u8 MaskB = 0b00000000;
    while((**Pointer) & Mask)
    {
        NBytes++;

        if(NBytes > 4)
        {
            (*Pointer)++;
            return 0xFFFD;
        }

        MaskB |= Mask;
        Mask >>= 1;
    }
    if(NBytes == 0) NBytes = 1;

    MaskB = ~MaskB;
    Result = (*Pointer)[0] & MaskB;
    (*Pointer)++;
    
    while(--NBytes)
    {
        Result <<= 6;
        Result |= (**Pointer) & 0b00111111;
        (*Pointer)++;
    }
    
    return Result;
}

u8 *codepoint_start(u64 codepoint_index, u8 *start, u8 *end)
{    
    u64 ix = 0;
    
    u8 *at = start;
    while(at < end) {
        if(ix == codepoint_index) return at;
        eat_codepoint(&at);
        ix++;
    }

    return at;
}

u32 eat_codepoint_backwards(u8 **at)
{
    u8 *cp_start = find_codepoint_backwards(*at);

    if(cp_start == NULL)
        return 0;

    *at = cp_start;
    return eat_codepoint(&cp_start);
}


u32 count_codepoints(String str, bool utf_16 = false)
{
    u32 result = 0;
    
    u8 *at = str.data;
    u8 *end = at + str.length;
    
    while(at < end)
    {
        eat_codepoint(&at, utf_16);
        result++;
    }

    return result;
}


String eat_unicode_word_or_symbol(u8 **at, u8 *end, int *_last_codepoint = NULL)
{
    String result = {0};
    result.data = *at;

    int cp = 0;
    while(*at < end)
    {
        cp = eat_codepoint(at);
        Assert(cp != 0);
        
        if(is_whitespace_or_symbol(cp))
        {
            if(result.length == 0)
                result.length = 1;
            else
            {
                if(is_symbol(cp)) // Let the first symbol be included in the word
                    result.length++;
                else
                    (*at)--;
            }
            
            break;
        }
        result.length = *at - result.data;
    }
    
    if(_last_codepoint)
        *_last_codepoint = cp;

    return result;
}

// NOTE: Returns number of bytes
int utf8_encode(u32 Codepoint, u8 **Pointer)
{
    int num_encode_bytes = 1;
    if (Codepoint & 0xFFFFFF80)
    {
        num_encode_bytes++;
        if (Codepoint & 0xFFFFF800)
        {
            num_encode_bytes++;
            if (Codepoint & 0xFFFF8000)
            {
                num_encode_bytes++;
            }
        }

    }
    
    if(num_encode_bytes == 1)
    {
        **Pointer = (u8)Codepoint;
        (*Pointer)++;
        return num_encode_bytes;
    }

    u8 Bytes[4] = {0};
    
    if(num_encode_bytes > 1)
        for(int B = 0; B < num_encode_bytes; B++)
            Bytes[0] = (Bytes[0] >> 1) | (0x1 << 7);

    u8 num_encode_bits_in_first_encode_byte = (num_encode_bytes > 1) ? (8 - num_encode_bytes - 1) : (8);
    
    int num_encode_bits = num_encode_bits_in_first_encode_byte + 6 * (num_encode_bytes - 1);

    //From left to right
    for(int BitIX = 0; BitIX < num_encode_bits; BitIX++)
    {
        //Place the bit at left-most position
        u32 codepoint_bit_mask = 0x1 << ((num_encode_bits - 1) - BitIX);
        u32 codepoint_bit = (Codepoint & codepoint_bit_mask); // "Align" it to the right if we have more bits than we need.
        u32 encode_bit = codepoint_bit;
        
        if(BitIX < (num_encode_bits_in_first_encode_byte))
            Bytes[0] |= encode_bit >> ((num_encode_bytes - 1) * 6);
        else
        {
            int ByteIndex = 1 + (BitIX - num_encode_bits_in_first_encode_byte) / 6;
            Bytes[ByteIndex] |= encode_bit >> (6 * (num_encode_bytes - 1 - ByteIndex));

            //@Speed: Do this once?
            Bytes[ByteIndex] |= 0x80;
        }
    }

    for(int B = 0; B < num_encode_bytes; B++)
    {
        **Pointer = Bytes[B];
        (*Pointer)++;
    }

    return num_encode_bytes;
}

inline
int utf8_encode(u32 codepoint, u8 *buffer)
{
    u8 *at = buffer;
    int result = utf8_encode(codepoint, &at);
    Assert(at >= buffer && at <= buffer + 4);
    return result;
}


void UTF8EncodeFromUTF16(String *string)
{
    u8 *At = (u8 *)string->data;
    u8 *End = At + string->length;

    u8 *Dest = At;
    while(At < End)
    {
        u32 Codepoint = eat_codepoint(&At, true);
        utf8_encode(Codepoint, &Dest);
    }

    string->length = Dest - (u8 *)string->data;
}


