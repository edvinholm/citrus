
typedef u64 UI_ID;

const u64 MAX_ID_PATH_LENGTH = 128; // 128 * 8B = 1024B

/*****
    STRUCTURE OF Location (u64)
    {
        u16 file; // Index in UI_ID_Manager.file_list PLUS ONE!! IMPORTANT
        u16 line; // @Robustness: We can not have files longer than 65535 lines.
        u32 count;
    };
*****/
typedef u64 UI_Location;
struct UI_Path
{
    u64 length;
    UI_Location locations[MAX_ID_PATH_LENGTH];
};

struct UI_ID_Manager
{
    u8 *file_list_start;    // Zero-terminated strings after one another. Allocated with ALLOC_UI.
    u8 *file_list_end;      // Last zero included.
    u64 file_list_capacity; // In bytes

    struct Table
    {
        struct Entry
        {
            //TODO @Incomplete: @Speed Make buckets here, where we store the paths after each other, terminating them with 64 zero bits.
            //                         We should have buckets because we don't want to copy all the paths when we run out of space.
            //                         Should be pretty big buckets though?

            Array<UI_Path,  ALLOC_UI> paths;
            Array<UI_ID, ALLOC_UI> ids;
        };

        Entry entries[65535]; // Hash is 16 bits, and created from a Path.
    };
};

struct UI_Manager
{
    UI_ID_Manager id_manager;

    UI_Path current_path;
};


inline
void assert_state_valid(UI_Manager *ui)
{
    auto &id_manager = ui->id_manager;
}

//NOTE: file needs to be zero-terminated.
//NOTE: Returns false if not found.
//NOTE: If this function returns false, *_id will be set to the id the next file added to the list will have.
inline
bool find_ui_file_id(u8 *file, UI_Manager *ui, u16 *_id)
{
    UI_ID_Manager *manager = &ui->id_manager;
    
    *_id = 1;

    if (manager->file_list_capacity == 0) return false;
    
    u8 *file_at = file;
    
    u8 *at  = manager->file_list_start;
    u8 *end = manager->file_list_end;
    while(at < end) {

        u8 ch = *at++;
        
        if(ch != *file_at) {
            
            // Skip string
            while(at < end)
                if(*at++ == 0) break;

            // Back to the start!
            Assert(*_id < U16_MAX);
            file_at = file;
            (*_id)++;
            continue;
        }

        if(ch == 0) { // End of string in list
            
            if(ch == *file_at) { // We found our file!
                return true;
            }

            // Not our file.
            // Back to the start!
            Assert(*_id < U16_MAX);
            file_at = file;
            (*_id)++;
            continue;
        }

        file_at++;
    }

    // Not found.
    return false;
}

inline
u16 find_or_create_file_id(u8 *file, UI_Manager *ui)
{
    assert_state_valid(ui);

    UI_ID_Manager *idm = &ui->id_manager;

    u16 id;
    if(find_ui_file_id(file, ui, &id)) return id;
    // id should now be set to the new file's id.

    
    // CREATE NEW //
    
    strlength file_length = cstring_length(file) + 1;              // Terminating zero included.
    u64 old_file_list_length = (idm->file_list_end - idm->file_list_start); // Last zero included.
    
    ensure_capacity(&idm->file_list_start, &idm->file_list_capacity, old_file_list_length + file_length, ALLOC_UI, (u64)256, true);
    idm->file_list_end = idm->file_list_start + old_file_list_length;

    memcpy(idm->file_list_end, file, file_length);
    idm->file_list_end += file_length;

    return id;
}


u16 ui_path_hash(UI_Path *path)
{
    Assert(path->length < MAX_ID_PATH_LENGTH);
    u16 hash = 0;
    for(u64 l = 0; l < path->length; l++)
    {
        UI_Location loc = path->locations[l];

        u16 file = (loc >> 48) & 0xFFFF;
        hash ^= (file % 0xF) << 12;
        
        hash ^= (loc >> (48+8)) & 0x00FF; // file (swap upper and lower because lower will probably more often be non-zero)
        hash ^= (loc >> (48-8)) & 0xFF00; // file (swap upper and lower because lower will probably more often be non-zero)
        hash ^= (loc >> (32-0)) & 0xFFFF; // line
        hash ^= (loc >> (16-0)) & 0xFFFF; // count
        hash ^= (loc >> ( 0-0)) & 0xFFFF; // count
    }

    return hash;
}

void push_ui_location(char *file, u16 line, u32 count, UI_Manager *ui)
{
    assert_state_valid(ui);

    // The conversion to u64 here is intentional.
    u64 file_id = find_or_create_file_id((u8 *)file, ui);
    Assert(file_id != 0);

    // Make location
    UI_Location loc = (file_id << 48) | ((u64)line << 32) | ((u64)count << 0);

    // Push location
    Assert(ui->current_path.length < MAX_ID_PATH_LENGTH);
    ui->current_path.locations[ui->current_path.length++] = loc;

#if DEBUG
    Debug_Print("\nCurrent path:\n");
    Debug_Print("[Hash: %04X]\n", ui_path_hash(&ui->current_path));
    for(u64 i = 0; i < ui->current_path.length; i++) {
        if(i > 0) Debug_Print("      >> ");
        Debug_Print("%I64X", ui->current_path.locations[i]);
    }
    Debug_Print("\n");
    for(u64 i = 0; i < ui->current_path.length; i++) {
        if(i > 0) Debug_Print(" >> ");

        UI_Location l = ui->current_path.locations[i];
        
        u16 file  = (l >> 48) & 0xFFFF;
        u16 line  = (l >> 32) & 0xFFFF;
        u16 count = (l >>  0) & 0xFFFFFFFF;
        
        Debug_Print("%04X|%04X|%08X", file, line, count);
    }
    Debug_Print("\n\n");
#endif
}
