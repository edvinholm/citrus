
struct Developer
{
    String user_id;
};

void init_developer(Developer *dev)
{
    const char *filename = "local.dev_id";
    
    FILE *file = open_file((char *)filename, false);
    if(!file) {
        Debug_Print("Unable to open %s.\n", filename);
        return;
    }

    strlength user_id_length;
    if(!read_entire_file(file, &dev->user_id.data, ALLOC_DEV, &user_id_length)) {
        Debug_Print("Unable to read %s.\n", filename);
        return;
    }

    dev->user_id.length = user_id_length;
}
