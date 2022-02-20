
enum Sprite_ID
{
    #include "gen/sprite_ids.cpp"
};

String sprite_names[] = {
    #include "gen/sprite_names.cpp"
};
static_assert(ARRLEN(sprite_names) == SPRITE_NONE_OR_NUM);

Sprite sprites[] = {
    #include "gen/sprite_definitions.cpp"
};
static_assert(ARRLEN(sprites) == SPRITE_NONE_OR_NUM);



enum Mesh_ID
{
    MESH_BED,
    MESH_CHAIR,
    MESH_BLENDER,
    MESH_TABLE,
    MESH_BARREL,
    MESH_FILTER_PRESS,
    MESH_STOVE,
    MESH_GRINDER,
    MESH_TOILET,

    MESH_APPLE_TREE_10,
    MESH_APPLE_TREE_25,
    MESH_APPLE_TREE_50,
    MESH_APPLE_TREE_75,
    MESH_APPLE_TREE_100,
    MESH_APPLE_TREE_100_FRUIT,

    MESH_FENCE,
    MESH_STREET_LIGHT,
    MESH_AWNING,
    MESH_FOUNTAIN,
    MESH_DOOR,
    MESH_WINDOW,
    MESH_FLOWER_BOX_WALL,

    MESH_SIGN_CHESS,
    
    MESH_NONE_OR_NUM
};

struct Asset_Catalog
{
    // MESHES //
    Mesh meshes     [MESH_NONE_OR_NUM] = { 0 };
    VAO  mesh_vaos  [MESH_NONE_OR_NUM] = { 0 };
    bool mesh_loaded[MESH_NONE_OR_NUM] = { 0 };
    static_assert(ARRLEN(meshes) == ARRLEN(mesh_vaos));
    static_assert(ARRLEN(meshes) == ARRLEN(mesh_loaded));
    // ////// //

    Linear_Allocator allocator;
};
void clear(Asset_Catalog *cat) {
    clear(&cat->allocator);
}

char *mesh_filenames[] = {
    "res/meshes/bedSingle.obj",
    "res/meshes/chair.obj",
    "res/meshes/blender.obj",
    "res/meshes/table.obj",
    "res/meshes/barrel.obj",
    "res/meshes/filter_press.obj",
    "res/meshes/kitchenStove.obj",
    "res/meshes/grinder.obj",
    "res/meshes/toiletSquare.obj",

    "res/meshes/apple_tree_10.obj",
    "res/meshes/apple_tree_25.obj",
    "res/meshes/apple_tree_50.obj",
    "res/meshes/apple_tree_75.obj",
    "res/meshes/apple_tree_100.obj",
    "res/meshes/apple_tree_100_fruit.obj",

    "res/meshes/fence.obj",
    "res/meshes/street_light.obj",
    "res/meshes/awning.obj",
    "res/meshes/fountain.obj",
    "res/meshes/door.obj",
    "res/meshes/window.obj",
    "res/meshes/flower_box_wall.obj",

    "res/meshes/sign_chess.obj"
};
static_assert(ARRLEN(mesh_filenames) == MESH_NONE_OR_NUM);
static_assert(ARRLEN(mesh_filenames) == ARRLEN(Asset_Catalog::meshes));





bool load_meshes(Asset_Catalog *cat)
{
    bool total_success = true;

    Array<u8, ALLOC_MALLOC> file_contents = {0};
    defer(clear(&file_contents););
    
    OBJ_Reader<ALLOC_TMP> obj_reader = {0};
    
    for(int i = 0; i < MESH_NONE_OR_NUM; i++) {
        Mesh *mesh     = &cat->meshes[i];
        char *filename = mesh_filenames[i];

        cat->mesh_loaded[i] = false;
        
        Zero(cat->mesh_vaos[i]);

        if(!read_entire_file(filename, &file_contents)) {
            Debug_Print("Unable to read contents of file '%s'\n", filename);
            total_success = false;
            continue;
        }

        if(!read_obj({file_contents.e, file_contents.n}, &obj_reader, &cat->allocator, mesh)) {
            Debug_Print("Unable to read OBJ data of file '%s'\n", filename);
            total_success = false;
            continue;
        }

        cat->mesh_loaded[i] = true;
    }

    return total_success;
}

bool load_assets(Asset_Catalog *cat)
{
    bool total_success = true;
    
    if(!load_meshes(cat)) total_success = false;

    return total_success;
}


void init_assets_for_drawing(Asset_Catalog *cat, Graphics *gfx)
{
    load_texture_catalog(&gfx->textures);
    
    for(int i = 0; i < ARRLEN(cat->meshes); i++) {
        if(!cat->mesh_loaded[i]) continue;

        Mesh *mesh = &cat->meshes[i];
        VAO  *vao   = &cat->mesh_vaos[i];
        
        *vao = create_vao();
        vao->vertex0 = 0;
        vao->vertex1 = mesh->n;
        push_vao_to_gpu(vao, mesh->positions, mesh->uvs, mesh->colors, mesh->tex, mesh->normals, gfx);
    }
}
