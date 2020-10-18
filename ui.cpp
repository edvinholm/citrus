
/*
  AUTOMATIC UI ID GENERATION
  --------------------------
  This is a messy solution, but it's the best way I've come
  up with for C++. And it's worth it to not need to worry
  about generating unique and reproducable IDs.

  It is this messy to help the programmer as much as possible and prevent them
  to do some of the most easily made mistakes. But it's nowhere near perfect,
  and you need to respect the rules of the systems in order for it to work.

  The same code path gives the same ID, basically.
  The paths are stored and mapped to ID integers.
  A path is a sequence of locations, identified by a "place" and a "count".
  The place component is an ID of a PACK site. The count is the number of
  packs in that place, in the current path...

  Example:     (The P() and PC() macros are the packing macros, and the U() macro is the unpack macro)
  void foo() {
      U(ctx);
      for(int i = 0; i < 10; i++)
      {
          button(PC(ctx, i)); // This is a "place" that will be hit multiple times for the same path, so we need to pass a "count" in to make 10 unique IDs here.
      }
  }
  void bar(UI_Context ctx) {
      U(ctx);
      foo(P(ctx));  // This is a "place"
      foo(P(ctx));  // This is a "place"
  }
  void main() {
      bar(P(ctx));  // This is a "place"
  }

  When you PACK a context, the current location (place+count) is pushed onto the path stack.
  When you UNPACK a context, a pop of that location is deferred to happen at the end of the scope.
                             THIS SHOULD BE DONE AT THE TOP OF THE PROCEDURES. Do not use the context
                             after the pop!
  
  IMPORTANT: When the UI_Context object is COPIED, we assume that it is passed by value to a procedure.
             So it is NOT ALLOWED to assign a UI_Context variable by copying a context by value.

 */



// PACK WITH COUNT
#define PC(CTX, COUNT)                          \
    ctx.pack(__COUNTER__+1, COUNT)

// PACK
#define P(CTX)                                  \
    PC(CTX, 0)

// UNPACK
#define U(CTX)                                  \
    CTX.set_state_to_unpacked();                \
    defer(pop_ui_location(CTX.manager);)


typedef u64 UI_ID;

const u64 MAX_ID_PATH_LENGTH = 128; // 128 * 8B = 1024B

/*****
    STRUCTURE OF Location (u64)
    {
        u32 place; // This is a unique ID for the place in the code (__COUNTER__) IMPORTANT: Must be >0.
        u32 count;
    };
*****/
typedef u64 UI_Location;
struct UI_Path
{
    UI_Location e[MAX_ID_PATH_LENGTH];
};

struct UI_ID_Manager
{
    UI_ID last_id;
    
    struct Path_Bucket
    {
        //TODO @Incomplete: @Speed Make buckets here, where we store the paths after each other, terminating them with 64 zero bits.
        //                         We should have buckets because we don't want to copy all the paths when we run out of space.
        //                         Should be pretty big buckets though?

        Array<u64,     ALLOC_UI> path_lengths;
        Array<UI_Path, ALLOC_UI> paths;
        Array<UI_ID,   ALLOC_UI> ids;
    };

    Path_Bucket path_buckets[256]; // Hash is 8 bits, and created from a Path.

    
};

struct UI_Manager
{
    UI_ID_Manager id_manager;

    UI_Path current_path;
    u64     current_path_length;
};

inline
void assert_state_valid(UI_ID_Manager *manager)
{
    
}

inline
void assert_state_valid(UI_Manager *ui)
{
    auto &id_manager = ui->id_manager;
    
    assert_state_valid(&id_manager);
}

u8 ui_path_hash(UI_Path *path, u64 length)
{    
    Assert(length < MAX_ID_PATH_LENGTH);
    u8 hash = 0;
    for(u64 l = 0; l < length; l++)
    {
        UI_Location loc = path->e[l];

        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF; loc >>= 8;
        hash ^= loc & 0xFF;
    }

    return hash;
}

UI_ID find_or_create_ui_id_for_path(UI_Path *path, u64 length, UI_ID_Manager *manager)
{
    u8 hash = ui_path_hash(path, length);

    Assert(hash < ARRLEN(manager->path_buckets));
    auto *bucket = manager->path_buckets + hash;

    Assert(bucket->path_lengths.n == bucket->paths.n);
    Assert(bucket->path_lengths.n == bucket->ids.n);
    for(u64 p = 0; p < bucket->path_lengths.n; p++)
    {
        if(bucket->path_lengths[p] != length) continue;

        bool found = true;
            
        auto &path_in_bucket = bucket->paths[p];
        for(u64 l = 0; l < length; l++) {
            
            Assert(path_in_bucket.e[l] != 0);
            
            if(path_in_bucket.e[l] != path->e[l]) {
                found = false;
                break;
            }
            
        }

        if(!found) continue;

        return bucket->ids[p];
    }


    // NOT FOUND, SO CREATE //

    UI_ID id = ++manager->last_id;
    Assert(id != 0);
    array_add(bucket->path_lengths,  length);
    array_add(bucket->paths,        *path);
    array_add(bucket->ids,           id);

    return id;
    
}

void push_ui_location(u32 place, u32 count, UI_Manager *ui)
{
    Assert(place != 0);
    
    assert_state_valid(ui);


    // Make location
    UI_Location loc = ((u64)place << 32) | ((u64)count << 0);

    // Push location
    Assert(ui->current_path_length < MAX_ID_PATH_LENGTH);
    ui->current_path.e[ui->current_path_length++] = loc;

#if DEBUG && false
    Debug_Print("\nCurrent path:\n");
    Debug_Print("[Hash: %02X]\n", ui_path_hash(&ui->current_path, ui->current_path_length));
    for(u64 i = 0; i < ui->current_path_length; i++) {
        if(i > 0) Debug_Print("      >> ");
        Debug_Print("%I64X", ui->current_path.e[i]);
    }
    Debug_Print("\n");
    for(u64 i = 0; i < ui->current_path_length; i++) {
        if(i > 0) Debug_Print(" >> ");

        UI_Location l = ui->current_path.e[i];
        
        u16 place = (l >> 32) & 0xFFFFFFFF;
        u16 count = (l >>  0) & 0xFFFFFFFF;
        
        Debug_Print("%08X|%08X", place, count);
    }
    Debug_Print("\n\n");
#endif
}

void pop_ui_location(UI_Manager *ui)
{
    Assert(ui->current_path_length > 0);
    ui->current_path_length--;
}




struct UI_Context
{
    UI_Manager *manager;
    
    enum State
    {
        UNPACKED  = 0,
        PACKED    = 1,
        DELIVERED = 2
    };
    
    UI_Context()
    {
        Zero(*this);
    }

    UI_Context(const UI_Context &ctx)
    {
        memcpy(this, &ctx, sizeof(*this));
        
        if(returning_from_pack) {
            returning_from_pack = false;
            return;
        }

        if(this->state == PACKED) {
            this->state = DELIVERED;
            return;
        }
        else {
            printf("ERROR: UI_Context must be PACKED on copy.\n");
            Assert(false);
        }
    }


    UI_ID get_id()
    {
        Assert(state == UNPACKED);
        return find_or_create_ui_id_for_path(&manager->current_path, manager->current_path_length, &manager->id_manager);
    }

    
    UI_Context pack(u32 place, u32 count)
    {
        Assert(!returning_from_pack);
        
        UI_Context ctx = UI_Context();
    
        if(this->state != UNPACKED) {
            printf("ERROR: UI_Context must be unpacked on pack().\n");
            Assert(false);
        }
    
        memcpy(&ctx, this, sizeof(*this));

        push_ui_location(place, count, this->manager);
    
        // NOTE: Unfortunately, when returning the context, the copy constructor for UI_Context will be called
        //       To prevent that constructor from thinking that this is a parameter-pass-to-proc,
        //       we set a special state (PACKING) here, and the constructor will then set it to PACKED, and not DELIVERED.
        ctx.state = PACKED;

        ctx.returning_from_pack = true;
        return ctx;
    }

    // IMPORTANT: THIS IS NOT A FULL UNPACK. Use the UNPACK() macro!
    void set_state_to_unpacked() {
        Assert(!returning_from_pack);
        
        state = UNPACKED;
    }
    
    State get_state() { return this->state; }


    ~UI_Context()
    {
        if(returning_from_pack) return;
        
        if(state != UNPACKED) {
            Debug_Print("All UI_Contexts must be UNPACKED before being destroyed! A context is being deconstructed with state set to DELIVERED, which should never happen.\n");
            Assert(false); // All delivered contexts should be unpacked.
        }
    }


private:    
    State state;
    bool returning_from_pack;
};





void button(UI_Context ctx)
{
    U(ctx);
    
    UI_Manager *ui = ctx.manager;
    
    Debug_Print("Button ID: %I64X\n", ctx.get_id());
}
