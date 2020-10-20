


bool read_from_file(byte *_data, s32 length, FILE *file)
{
    return (fread(_data, length, 1, file) == 1);
}

inline
bool read_s32(s32 *_i, FILE *file)
{
    s32 i;
    if(!read_from_file((byte *)&i, 4, file)) return false;
    *_i = machine_endian_from_big_32(i);
    return true;
}


inline
bool read_u32(u32 *_i, FILE *file)
{
    u32 i;
    if(!read_from_file((byte *)&i, 4, file)) return false;
    *_i = machine_endian_from_big_32(i);
    return true;
}
