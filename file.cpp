

inline
bool close_file(FILE *file)
{
    return (fclose(file) == 0);
}

inline
FILE *open_file(char *filename, bool for_writing)
{
    return fopen(filename, (for_writing) ? "wb" : "rb");
}


// @Cleanup @Robustness: Don't close the file in this proc. Makes more sense to both open and close the file outside of it.
bool read_entire_file(FILE *File, u8 **_Data, Allocator_ID allocator, u32 *_Length = NULL)
{
    Assert(File);
 
    fseek(File, 0, SEEK_END);
    long FileSize = ftell(File);
    rewind(File);
    
    *_Data = (byte *)alloc(FileSize+1, allocator);
    size_t FReadResult = fread(*_Data, 1, FileSize, File);
    if(FReadResult != FileSize)
    {
        close_file(File);
        dealloc_if_legal(*_Data, allocator);
        *_Data = NULL;
        return false;
    }

    (*_Data)[FileSize] = 0;

    close_file(File);
    
    if(_Length) *_Length = FileSize;
    
    return true;
}

bool read_entire_file(char *Filename, u8 **_Data, Allocator_ID allocator, u32 *_Length = 0)
{
    FILE *File = fopen(Filename, "rb");
    if(!File) return false;

    return read_entire_file(File, _Data, allocator, _Length);
}

inline
bool read_entire_file(char *filename, String *_output, Allocator_ID allocator)
{
    return read_entire_file(filename, (byte **)&_output->data, allocator, (u32 *)&_output->length);
}

// NOTE: We could just use FILE for referring to resources, but we have this to make sure we don't pass them to the wrong procs.
struct Resource_Handle
{
    FILE *file;
};

inline
Resource_Handle open_resource(char *filename, bool for_writing = false, String_Builder *builder = NULL)
{
    #if OS_ANDROID
    
    return { platform_open_resource(filename, for_writing) };
    
    #else
    
    static String_Builder default_builder = {0};
    if(!builder)
        builder = &default_builder;
    
    String resource_directory = platform_get_resource_directory();
    string_append(resource_directory, *builder);
    if(resource_directory[resource_directory.length-1] != '/')
        string_append_char('/', *builder);
    string_append(filename, *builder);
    filename = copy_built_string_as_cstring(*builder, ALLOC_TMP);

    Resource_Handle res = { 0 };
    res.file = open_file(filename, for_writing);
    return  res;
    
    #endif
}

inline
bool close_resource(Resource_Handle handle)
{
#if !(OS_ANDROID)
    return close_file(handle.file);
#else
    return platform_resource_close(handle.file);
#endif
}


inline
FILE *open_user_file(char *filename, bool for_writing = false, String_Builder *builder = NULL)
{    
    static String_Builder default_builder = {0};
    if(!builder)
        builder = &default_builder;
    
    String user_directory = platform_get_user_directory();
    string_append(user_directory, *builder);
    if(user_directory[user_directory.length-1] != '/')
        string_append_char('/', *builder);
    string_append(filename, *builder);
    filename = copy_built_string_as_cstring(*builder, ALLOC_TMP);

    Debug_Print("Opening user file '%s'\n", filename);

    FILE *file = open_file(filename, for_writing);

    Debug_Print("open_file %s (%d)\n", (file) ? "success" : "fail", errno);
    
    if(!file) return NULL;
    return file;
}



bool resource_read(Resource_Handle resource, u8 *_data, s32 size) {
    
#if OS_ANDROID
    return platform_resource_read(resource.file, _data, size);
#else
    return (fread(_data, size, 1, resource.file) == 1);
#endif
    
}

bool resource_write(Resource_Handle resource, u8 *data, s32 size) {
    
#if OS_ANDROID
    return platform_resource_write(resource.file, data, size);
#else
    return (fwrite(data, size, 1, resource.file) == 1);
#endif
    
}

long resource_seek(Resource_Handle resource, long offset, int whence) {
    
#if OS_ANDROID
    return platform_resource_seek(resource.file, offset, whence);
#else
    return fseek(resource.file, offset, whence);
#endif
    
}

// @Startup: @Speed: Pass a builder here always so we don't allocate memory every time.
inline
bool read_entire_resource(char *filename, byte **_data, Allocator_ID allocator,
                          u32 *_length = NULL, String_Builder *builder = NULL)
{   
    Resource_Handle resource = open_resource(filename, false, builder);

    if(!resource.file) return false;
    
#if OS_ANDROID
    
    defer(close_resource(resource););
    
    Assert(resource.file);

    size_t size = resource_seek(resource, 0, SEEK_END);
    resource_seek(resource, 0, SEEK_SET);
    
    *_data = (byte *)alloc(size+1, allocator);
    if(!resource_read(resource, *_data, size))
    {
        dealloc_if_legal(*_data, allocator);
        *_data = NULL;
        return false;
    }
    
    (*_data)[size] = 0;
    
    if(_length) *_length = size;

    return true;

#else

    return read_entire_file(resource.file, _data, allocator, _length);
        
#endif




    return true;
}


