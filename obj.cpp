
struct OBJ_Face
{
    v3s vertices[3];
};

template<Allocator_ID A>
struct OBJ_Reader
{
    v3 *positions;
    int num_positions;
    
    v3 *normals;
    int num_normals;

    v2 *uvs;
    int num_uvs;

    OBJ_Face *faces;
    int num_faces;

    u64 capacity;

    Mesh mesh;
    u64 mesh_capacity;
};

void skip_whitespace(u8 **at, u8 *end, bool skip_newlines)
{
    while(*at < end) {
        u8 ch = **at;
        if(ch == ' ' || (skip_newlines && (ch == '\n' || ch == '\r')))  {
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

bool eat_token(u8 **at, u8 *end, String *_token, bool skip_newlines)
{
    skip_whitespace(at, end, skip_newlines);

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


template<Allocator_ID A>
void ensure_obj_reader_capacity(u64 required_capacity, OBJ_Reader<A> *reader)
{
    // @Speed
    void **buffers[] = {
        (void **)&reader->positions,
        (void **)&reader->normals,
        (void **)&reader->uvs,
        (void **)&reader->faces
    };

    size_t buffer_element_sizes[] = {
        sizeof(*reader->positions),
        sizeof(*reader->normals),
        sizeof(*reader->uvs),
        sizeof(*reader->faces)
    };
    //--

    static_assert(ARRLEN(buffers) == ARRLEN(buffer_element_sizes));
    ensure_buffer_set_capacity(required_capacity, &reader->capacity, buffers, buffer_element_sizes, ARRLEN(buffers), allocators[A]);
}


bool read_obj_index(u8 **at, u8 *end, int *_out)
{
    skip_whitespace(at, end, false);

    u8 *start = *at;
    *_out = strtol((const char *)*at, (char **)at, 10); // @Robustness.

    return (*at > start); // @Robustness: This is not how you chack if strtol was successful.
}


bool read_obj_vector_component(u8 **at, u8 *end, float *_out)
{
    skip_whitespace(at, end, false);

    u8 *start = *at;
    *_out = strtof((const char *)*at, (char **)at); // @Robustness.

    return (*at > start); // @Robustness: This is not how you chack if strtof was successful.
}

bool read_obj_v3(u8 **at, u8 *end, v3 *_v)
{
    if(!read_obj_vector_component(at, end, &_v->x)) return false;
    if(!read_obj_vector_component(at, end, &_v->y)) return false;
    if(!read_obj_vector_component(at, end, &_v->z)) return false;
    return true;
}

bool read_obj_v2(u8 **at, u8 *end, v2 *_v)
{
    if(!read_obj_vector_component(at, end, &_v->x)) return false;
    if(!read_obj_vector_component(at, end, &_v->y)) return false;
    return true;
}


template<Allocator_ID A>
bool read_obj_position(u8 **at, u8 *end, OBJ_Reader<A> *reader)
{
    v3 p;
    if(!read_obj_v3(at, end, &p)) return false;
    
    ensure_obj_reader_capacity(reader->num_positions + 1, reader);
    reader->positions[reader->num_positions++] = p;

    return true;
}

template<Allocator_ID A>
bool read_obj_normal(u8 **at, u8 *end, OBJ_Reader<A> *reader)
{
    v3 n;
    if(!read_obj_v3(at, end, &n)) return false;
       
    ensure_obj_reader_capacity(reader->num_normals + 1, reader);
    reader->normals[reader->num_normals++] = n;

    return true;
}

template<Allocator_ID A>
bool read_obj_uv(u8 **at, u8 *end, OBJ_Reader<A> *reader)
{
    v2 uv;
    if(!read_obj_v2(at, end, &uv)) return false;
       
    ensure_obj_reader_capacity(reader->num_uvs + 1, reader);
    reader->uvs[reader->num_uvs++] = uv;

    return true;
}

template<Allocator_ID A>
bool read_obj_face(u8 **at, u8 *end, OBJ_Reader<A> *reader)
{
    OBJ_Face face;

    for(int v = 0; v < 3; v++) {

        auto &vertex = face.vertices[v];
        
        for(int i = 0; i < 3; i++) {

            if(i > 0) {
                skip_whitespace(at, end, false);
                if(*at >= end || **at != '/') return false;
                (*at)++;
            }
        
            if(!read_obj_index(at, end, &vertex.comp[i])) return false;
        }
    }
    
    ensure_obj_reader_capacity(reader->num_faces + 1, reader);
    reader->faces[reader->num_faces++] = face;

    return true;
}


template<Allocator_ID Reader_Allocator>
bool read_obj(String contents, OBJ_Reader<Reader_Allocator> *reader, Allocator *mesh_allocator, Mesh *_mesh)
{
    reader->num_positions = 0;
    reader->num_normals   = 0;
    reader->num_faces     = 0;
    
    u8 *at  = contents.data;
    u8 *end = contents.data + contents.length;
    
    while(at < end) {

        String token;
        if(!eat_token(&at, end, &token, true)) break;

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
            if(!read_obj_position(&at, end, reader)) return false;
        }
        else if(equal(token, "vn")) {
            if(!read_obj_normal(&at, end, reader)) return false;
        }
        else if(equal(token, "vt")) {
            if(!read_obj_uv(&at, end, reader)) return false;
        }
        else if(equal(token, "usemtl")) {
            // @Norelease @Incomplete
            skip_line(&at, end);
        }
        else if(equal(token, "f")) {
            if(!read_obj_face(&at, end, reader)) return false;
        }
        else {
            Debug_Print("Unknown beginning of line: '%.*s'.\n", (int)token.length, token.data);
        }
    }

    for(int i = 0; i < min(32, reader->num_faces); i++) {
        Debug_Print("f ");
        for(int v = 0; v < 3; v++) {
            Debug_Print("%d/%d/%d  ", reader->faces[i].vertices[v].x, reader->faces[i].vertices[v].y, reader->faces[i].vertices[v].z);
        }
        Debug_Print("\n");
    }


    ensure_mesh_capacity(reader->num_faces * 3, &reader->mesh_capacity, &reader->mesh, allocators[Reader_Allocator]);
    reader->mesh.n = reader->num_faces * 3;
    
    for(int i = 0; i < reader->num_faces; i++)
    {
        auto *face = &reader->faces[i];
        for(int j = 0; j < 3; j++)
        {
            auto *vertex = &face->vertices[j];
            auto ix = i * 3 + j;

            // @Incomplete: We don't detect if positon/uv/normal is unspecified, like  f 1//3
            
            reader->mesh.positions[ix] = reader->positions[vertex->x-1];
            reader->mesh.uvs[ix]       = reader->uvs      [vertex->y-1];
            reader->mesh.normals[ix]   = reader->normals  [vertex->z-1];
        }
    }
    

    // @Incomplete: Faces are defined position/uv/normal.
    //              Number of positions, number of uvs and number of normals do not need to match.
    //
    //              Let's do it the non-@Speedy way for now, and instead of using vertex indices in
    //              the shaders, add one position, one uv and one normal to the mesh for each vertex
    //              for each face.
    //             
  
    *_mesh = copy_mesh(&reader->mesh, mesh_allocator);
    return true;
}
