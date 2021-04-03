
struct OBJ_Face
{
    v3s vertices[3];
    v4  color;
};

struct MTL_Material
{
    v3 ambient_color;
    v3 diffuse_color;
    v3 specular_color;
    v3 emission_color;
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

    Array<MTL_Material, A> materials;
    Array<String, A>       material_names;

    MTL_Material *current_material;

    Array<u8, A> file_contents; // Used by load_mtl().
};

// @Leak:
// @Leak:
// @Leak:
// OBJ_Reader is written assuming we use ALLOC_TMP for everything.
// To use another allocator, we would need to go through the code and look for leaks,
// and add a clear() procedure that we call when we are done with an OBJ_Reader.

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
            
            skip_whitespace(at, end, false);
            if(**at == '/') {
                vertex.comp[i] = 0;
                continue; // The index is unspecified.
            }
        
            if(!read_obj_index(at, end, &vertex.comp[i])) return false;
        }
    }

    face.color.rgb = (reader->current_material) ? reader->current_material->diffuse_color : V3_ONE;
    face.color.a   = 1.0f;
    
    ensure_obj_reader_capacity(reader->num_faces + 1, reader);
    reader->faces[reader->num_faces++] = face;

    return true;
}

template<Allocator_ID A>
MTL_Material *find_material(String name, OBJ_Reader<A> *reader)
{
    for(int i = 0; i < reader->material_names.n; i++) {
        if(equal(reader->material_names[i], name)) return &reader->materials[i];
    }
    return NULL;
}



// NOTE: This replaces the OBJ_Reader's materials.
//       These materials will then be used when loading meshes with this reader.
template<Allocator_ID Reader_Allocator>
bool read_mtl(String contents, OBJ_Reader<Reader_Allocator> *reader)
{
    reader->materials.n      = 0;
    reader->material_names.n = 0;

    u8 *at  = contents.data;
    u8 *end = contents.data + contents.length;
    
    while(at < end) {
        String token;
        if(!eat_token(&at, end, &token, true)) break;

        if(equal(token, "#")) {
            skip_line(&at, end);
        }
        else if(equal(token, "newmtl")) {
            // Parse material
            String name;
            if(!eat_token(&at, end, &name, true)) {
                Debug_Print("ERROR: Expected name of material.\n");
                return false;
            };

            MTL_Material material = {0};
            
            while(at < end) {
                u8 *at_before_eat = at;
                
                String token;
                if(!eat_token(&at, end, &token, true)) break;

                if(equal(token, "newmtl")) {
                    at = at_before_eat; // Undo eat.
                    break;
                }

                if(equal(token, "Ns")) {
                    skip_line(&at, end);
                } else if(equal(token, "Ka")) {
                    if(!read_obj_v3(&at, end, &material.ambient_color)) return false;
                } else if(equal(token, "Kd")) {
                    if(!read_obj_v3(&at, end, &material.diffuse_color)) return false;
                } else if(equal(token, "Ks")) {
                    if(!read_obj_v3(&at, end, &material.specular_color)) return false;
                } else if(equal(token, "Ke")) {
                    if(!read_obj_v3(&at, end, &material.emission_color)) return false;
                } else if(equal(token, "Ns")) {
                    skip_line(&at, end);
                } else if(equal(token, "Ni")) {
                    skip_line(&at, end);
                } else if(equal(token, "d")) {
                    skip_line(&at, end);
                } else if(equal(token, "illum")) {
                    skip_line(&at, end);
                } else {
                    Debug_Print("MTL: NOTE: Unexpected token at beginning of line (inside material): '%.*s'.\n", (int)token.length, token.data);
                    skip_line(&at, end);
                }
            }

            // @Leak!! If Reader_Allocator != ALLOC_TMP.
            array_add(reader->material_names, copy_of(&name, Reader_Allocator));
            array_add(reader->materials,      material);
        }
        else {
            Debug_Print("MTL: NOTE: Unexpected token at beginning of line: '%.*s'.\n", (int)token.length, token.data);
            skip_line(&at, end);
        }
    }

    return true;
}


template<Allocator_ID A>
bool load_mtl(char *filename, OBJ_Reader<A> *reader)
{
    if(read_entire_file(filename, &reader->file_contents)) {
        return read_mtl({reader->file_contents.e, reader->file_contents.n}, reader);
    } else {
        Debug_Print("Unable to read contents of file '%s'\n", filename);
        return false;
    }
}

template<Allocator_ID Reader_Allocator>
bool read_obj(String contents, OBJ_Reader<Reader_Allocator> *reader, Allocator *mesh_allocator, Mesh *_mesh)
{
    reader->num_positions = 0;
    reader->num_normals   = 0;
    reader->num_faces     = 0;
    
    u8 *at  = contents.data;
    u8 *end = contents.data + contents.length;

    reader->current_material = NULL;
    
    while(at < end) {

        String token;
        if(!eat_token(&at, end, &token, true)) break;

        if(equal(token, "#")) {
            skip_line(&at, end);
        }
        else if(equal(token, "mtllib")) {
            String name;
            if(!eat_token(&at, end, &name, true)) {
                Debug_Print("ERROR: Expected name of material library.\n");
                return false;
            };

            char *filename = concat_cstring_tmp("res/meshes/", name);
            if(!load_mtl(filename, reader)) {
                Debug_Print("ERROR: Unable to find material library '%.*s'.\n", (int)name.length, name.data);
                return false;
            }
        }
        else if(equal(token, "g")) {
            skip_line(&at, end);
        }
        else if(equal(token, "o")) {
            skip_line(&at, end);
        }
        else if(equal(token, "s")) {
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

            String name;
            if(!eat_token(&at, end, &name, true)) {
                Debug_Print("ERROR: Expected name of material.\n");
                return false;
            };

            reader->current_material = find_material(name, reader);
            if(reader->current_material == NULL) {
                Debug_Print("NOTE: Material '%.*s' not loaded.\n", (int)name.length, name.data);
            }
        }
        else if(equal(token, "f")) {
            if(!read_obj_face(&at, end, reader)) return false;
        }
        else {
            Debug_Print("NOTE: Unknown beginning of line: '%.*s'.\n", (int)token.length, token.data);
            skip_line(&at, end);
        }
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

            reader->mesh.positions[ix] = (vertex->x > 0) ? reader->positions[vertex->x-1] : V3_ZERO;
            reader->mesh.uvs[ix]       = (vertex->y > 0) ? reader->uvs      [vertex->y-1] : V2_ZERO;
            reader->mesh.normals[ix]   = (vertex->z > 0) ? reader->normals  [vertex->z-1] : V3_ZERO;
            reader->mesh.colors[ix]    = face->color;
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

