

inline
GPU_Texture_ID create_texture_on_gpu(u32 w, u32 h, GPU_Texture_Parameters *_params = NULL, GPU_Error_Code *_error_code = NULL)
{
    #if GFX_GL
        GPU_Texture_Parameters params = gpu_texture_parameters(GPU_PIX_COMP_RGBA, GPU_PIX_FORMAT_RGBA, GPU_PIX_DATA_UNSIGNED_BYTE);
    #elif GFX_METAL
        GPU_Texture_Parameters params = gpu_texture_parameters(GPU_PIX_FORMAT_RGBA8_UNORM);
    #else
    #error(Unsupported Graphics API)
    #endif
    
    GPU_Texture_ID id;
    gpu_create_texture(w, h, params, &id, _error_code);

    if(_params) *_params = params;
    
    return id;
}


inline
void upload_texture_to_gpu(GPU_Texture_ID texture, u32 w, u32 h, Pixel *pixels,
                           GPU_Texture_Parameters params)
{
    gpu_set_texture_data(texture, pixels, w, h, params);
}



inline
GPU_Texture_ID create_texture(Pixel *pixels, u32 w, u32 h, GPU_Texture_Parameters *_params, GPU_Error_Code *_error_code = NULL)
{
    TIMED_FUNCTION;
    
    Assert(pixels);

    GPU_Error_Code error_code;
    
    GPU_Texture_ID gpu_id = create_texture_on_gpu(w, h, _params, &error_code);
    if(_error_code) *_error_code = error_code;
    
    if(error_code != 0) {
        Assert(false);
        return gpu_id; // Failure
    }

    gpu_set_texture_data(gpu_id, pixels, w, h, *_params);

    return gpu_id;
}


bool load_and_create_texture_from_file_data(u8 *file_data, u32 file_data_size, GPU_Texture_ID *_gl_id, v2s *_size, GPU_Texture_Parameters *_params
#if DEBUG
                                                   , Pixel **_pixels
#endif
                                                   )
{    
    int w, h;
    int channels;
    
    Pixel *pixels = (Pixel *)stbi_load_from_memory(file_data, file_data_size, &w, &h, &channels, 4);
        
    if(!pixels)
    {
        Debug_Print("stbi unable to read image file.\n");
        return false; //proc_status(__LINE__, false);
    }


    GPU_Error_Code error_code;
    *_gl_id = create_texture(pixels, w, h, _params, &error_code);
    if(error_code != 0)
        return false; //proc_status(__LINE__, false, (s64)error_code);
    
    *_size = V2S(w, h);

#if DEBUG
    *_pixels = pixels;    
#else
    free(pixels);
#endif

    return true; //proc_status(__LINE__, true);
}


bool load_texture_catalog_from_image_files(Texture_Catalog *cat)
{
    
#if DEBUG
    Allocator_ID allocator = ALLOC_DEV;
#else
    Allocator_ID allocator = ALLOC_TMP;
#endif

    bool success = true;

    for(int t = 0; t < TEX_NONE_OR_NUM; t++)
    {
        if(texture_filenames[t] == NULL)
            continue;
        
        #if DEBUG
        Pixel *pixels = NULL;
        #endif
        
        {
            TIMED_BLOCK("Load and create texture");
            u8 *file_data;
            u32 file_data_size;
            if(!read_entire_resource((char *)texture_filenames[t], &file_data, ALLOC_TMP, &file_data_size))
            {
                Debug_Print("Unable to load texture resource '%s'\n", texture_filenames[t]);
                success = false;
            }
            else
            {
                if(!load_and_create_texture_from_file_data(file_data, file_data_size, &cat->ids[t], &cat->sizes[t], &cat->params[t]
#if DEBUG
                                                           , &pixels
#endif
                                                           ))
                {
                    return false;
                }
            }
        }
        
        #if DEBUG
        //Assert(pixels);
        cat->pixels[t] = pixels;
        #endif
        
        cat->exists[t] = true;
    }

    return success;
}

bool load_texture_catalog(Texture_Catalog *cat)
{
    TIMED_FUNCTION;

    return load_texture_catalog_from_image_files(cat);
}

struct Graphics;

float bound_slot_for_texture(Texture_ID texture, Graphics *gfx)
{
    if(texture == TEX_NONE_OR_NUM) return 0;
    
    Assert(gfx->num_bound_textures > texture && gfx->bound_textures[texture] == texture);    
    return (float)texture+1;
}
