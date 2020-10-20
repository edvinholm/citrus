
bool write_to_file(byte *data, s32 length, FILE *file)
{
    return (fwrite(data, length, 1, file) == 1);
}

bool write_cstring(char *cstring, FILE *file)
{
    return write_to_file((byte *)cstring, strlen(cstring), file);
}

inline
bool write_s32(s32 i, FILE *file)
{
    i = big_endian_32(i);
    if(!write_to_file((byte *)&i, 4, file)) return false;
    return true;
}

inline
bool write_u32(u32 i, FILE *file)
{
    i = big_endian_32(i);
    if(!write_to_file((byte *)&i, 4, file)) return false;
    return true;
}


bool write_string(String str, FILE *file)
{
    if(!write_s32(str.length, file)) return false;
    if(!write_to_file(str.data, str.length, file)) return false;

    return true;
}
