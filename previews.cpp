
const int previews_texture_size = 2048;
const int preview_cells_per_side = 16;
const float preview_cell_size = previews_texture_size / preview_cells_per_side;


// IMPORTANT: Set target framebuffer back after calling this!
void update_previews(Graphics *gfx)
{
    gpu_set_target_framebuffer(gfx->previews_framebuffer);
        
    gpu_set_depth_testing_enabled(true);
    gpu_set_depth_mask(true);

    gpu_set_clear_color(0, 0, 0, 0);
    gpu_clear_color_and_depth_buffer();

    for(int i = 0; i < ITEM_NONE_OR_NUM; i++) {
        Item item = {0};
        item.type = (Item_Type_ID)i;
        
        Item_Type *type = &item_types[item.type];
        v3 offs = {type->volume.x * 0.5f, type->volume.y * 0.5f, 0};
        Entity e = create_item_entity(&item, V3_ZERO + offs, axis_rotation(V3_Z, -TAU * .25f), 0);
        
        m4x4 projection = world_projection_matrix(V2_ONE, type->volume.x, type->volume.y, type->volume.z, 0, false);
        config_gpu_for_world(gfx, { 0, 0, 2048, 2048 }, projection);

        draw_entity(&e, 0, gfx, NULL, NULL, false, false, false, &gfx->previews_render_buffer);
        
        gpu_clear_depth_buffer();
        gpu_set_viewport((i % preview_cells_per_side) * preview_cell_size, (i / preview_cells_per_side) * preview_cell_size, preview_cell_size, preview_cell_size);

        draw_render_object_buffer(&gfx->previews_render_buffer.opaque, false, gfx);
        reset_render_object_buffer(&gfx->previews_render_buffer.opaque);
    }
}


void draw_item_preview(Item_Type_ID item_type, Rect a, Graphics *gfx)
{
    Assert(item_type >= 0 && item_type < ITEM_NONE_OR_NUM);
    
    Item_Type *type = &item_types[item_type];
    String label = type->name;
    Assert(label.length > 0);
    label.length = min(3, label.length);
    
    int preview_col = item_type % preview_cells_per_side;
    int preview_row = item_type / preview_cells_per_side;
    v2 uv0 = { preview_col / (float)preview_cells_per_side, preview_row / (float)preview_cells_per_side };
    v2 uv1 = uv0 + V2_XY * (1.0f / preview_cells_per_side);
    v2 uvs[] = {
        uv0, {uv0.x, uv1.y}, {uv1.x, uv0.y},
        {uv1.x, uv0.y}, {uv0.x, uv1.y}, uv1,
    };

    draw_rect(shrunken(a, a.w * 0.08f), C_WHITE, gfx, 0, uvs, TEX_PREVIEWS);

//    _draw_button_label(label, a, gfx);
}

