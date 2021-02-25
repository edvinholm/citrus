
v4 profiler_colors[] = {
    C_RED,
    C_GREEN,
    C_BLUE,
    C_YELLOW,
    C_CYAN,
    C_FUCHSIA,
    C_ORANGE,
    C_LIME,
    C_TURQUOISE,
    C_PURPLE
};


void draw_profile_range(int start_ix, int end_ix, Rect a, Profiler *prof, Graphics *gfx)
{
    Assert(start_ix >= 0 && start_ix <= ARRLEN(prof->frames));
    Assert(end_ix >= 0   && end_ix <= ARRLEN(prof->frames));
    Assert(end_ix >= start_ix);

    float segment_w = a.w / (float)(end_ix - start_ix);
    auto *start = &prof->frames[start_ix];
    auto *end = &prof->frames[end_ix];
    auto *at    = start;

    double Hz = (double)platform_performance_counter_frequency();
    float scale = tweak_float(TWEAK_PROFILER_YSCALE);
    float h_per_time_unit = a.h * (1.0f/Hz) * scale;

    s64 max_frame_duration = 0;

    float y0 = a.y;
    float xx = a.x;

    float y1 = y0 + a.h;
        
    at = start;
    while(at < end)
    {
        float yy = y0;

        s64 total_frame_duration = 0;
                        
        auto *frame = at;
        for(int i = 0; i < frame->num_nodes; i++)
        {
            auto *node = &frame->nodes[i];
            if(node->parent != NULL) continue; // Only do root nodes.

            v4 *color = &profiler_colors[i % ARRLEN(profiler_colors)];

            auto dur = (node->t1 - node->t0);
            float h = h_per_time_unit * dur;
            h = min(y1-yy, h);
            
            draw_rect_ps({ xx, yy }, { segment_w, h }, *color, gfx);

            total_frame_duration += dur;
            
            yy += h;
        }
        at++;

        if(total_frame_duration > max_frame_duration)
            max_frame_duration = total_frame_duration;
        
        xx += segment_w;
    }

    float max_line_y = y0 + max_frame_duration * h_per_time_unit;
    if(max_line_y+2 <= y1) {
        draw_rect_ps({0, max_line_y - 1 }, { a.w, 3 }, C_BLACK, gfx);
        draw_rect_ps({0, max_line_y     }, { a.w, 1 }, C_WHITE, gfx);

        double us = 1000000.0 * (max_frame_duration / Hz);
        double ms = 1000.0    * (max_frame_duration / Hz);
        draw_string(concat_tmp(us, "us / ", ms, "ms"), { a.x + 4, max_line_y - 4 }, FS_12, FONT_TITLE, C_WHITE, gfx);
    }
}

void get_profiler_rects(Rect a, Rect *_node_a, Rect *_frame_a, Rect *_zoom_a, Rect *_all_a)
{
    cut_top_off(&a, 32);

    *_node_a  = cut_top_off(&a, 96);
    *_frame_a = cut_top_off(&a, 96);
    
    *_zoom_a = top_half_of(a);
    *_all_a  = removed_top(a, _zoom_a->h);
}

Rect profiler_selection_rect(int sel0, int sel1, Rect a, Profiler *prof)
{
    Rect selection_a = a;
    selection_a.w  = (sel1 - sel0) * (a.w / (float)ARRLEN(prof->frames));
    selection_a.x += (sel0)        * (a.w / (float)ARRLEN(prof->frames));
    
    return selection_a;
}

int max_depth_of_profiler_node(Profiler_Node *node)
{
    if(node->num_children == 0) return 1;
    
    int max_depth = 0;
    Assert(node->num_children < ARRLEN(node->children));
    
    for(int i = 0; i < node->num_children; i++)
    {
        int depth = max_depth_of_profiler_node(node->children[i]);
        
        if(depth > max_depth) {
            max_depth = depth;
        }
    }

    Assert(max_depth >= 1);
    return max_depth + 1;
}

void draw_profiler_node(Profiler_Node *node, int node_ix, Rect a, Input_Manager *input, Graphics *gfx, bool *_do_draw_tooltip, String *_tooltip_text)
{
    s64 dur = (node->t1 - node->t0);
    if(dur == 0) return;
    
    draw_rect(a, profiler_colors[node_ix % ARRLEN(profiler_colors)], gfx);

    Rect child_a = { a.x, a.y + a.h, 0, a.h };
    s64 last_t1 = node->t0;
    for(int i = 0; i < node->num_children; i++)
    {
        auto *child = node->children[i];
        
        s64 child_dur = (child->t1 - child->t0);
        if(child_dur == 0) continue;

        child_a.x += a.w * ((float)(child->t0 - last_t1) / dur);
        child_a.w  = a.w * ((float)child_dur / dur);
        
        draw_profiler_node(child, node_ix + 1 + i, child_a, input, gfx, _do_draw_tooltip, _tooltip_text);

        last_t1 = child->t1;
        child_a.x += child_a.w;
    }
    
    if(!*_do_draw_tooltip)
    {
        bool do_draw_tooltip = false;
        String tooltip_text  = EMPTY_STRING;
    
        // TOOLTIP //
        if(point_inside_rect(input->mouse.p, a)) {

            const Font_Size fs   = FS_14;
            const Font_ID   font = FONT_TITLE;

            auto dur = (node->t1 - node->t0);
            double Hz = platform_performance_counter_frequency();
            double ms = (dur / Hz) * 1000.0;
            double us = (dur / Hz) * 1000000.0;
            double ns = (dur / Hz) * 1000000000.0;
            tooltip_text = concat_tmp(STRING(node->label), ": ", ns, "ns / ", us, "us / ", ms, "ms | Int: ", node->user_int, ", Float: ", node->user_float);
            
            do_draw_tooltip = true;
        }

        *_do_draw_tooltip = do_draw_tooltip;
        *_tooltip_text    = tooltip_text;
    }
}

void draw_profile_frame(Profiler_Frame *frame, Rect a, Input_Manager *input, Graphics *gfx, bool *_do_draw_tooltip, String *_tooltip_text)
{
    if(frame->num_nodes <= 0) return;
    
    s64 sum_of_root_node_durations = 0;
    int max_depth = 0;
    
    for(int i = 0; i < frame->num_nodes; i++) {
        auto *node = &frame->nodes[i];
        if(node->parent != NULL) continue;

        int depth = max_depth_of_profiler_node(node);
        if(depth > max_depth) max_depth = depth;

        auto dur = (node->t1 - node->t0);
        sum_of_root_node_durations += dur;
    }

    if(sum_of_root_node_durations <= 0) return;

    Assert(max_depth >= 1);
    
    float w_per_time_unit = a.w / sum_of_root_node_durations;

    Rect node_a = { a.x, a.y, 0, a.h/max_depth };
    auto node_ix = 0;

    for(int i = 0; i < frame->num_nodes; i++) {
        auto *node = &frame->nodes[i];
        if(node->parent != NULL) continue;

        auto dur = (node->t1 - node->t0);
        node_a.w = w_per_time_unit * dur;

        draw_profiler_node(node, node_ix, node_a, input, gfx, _do_draw_tooltip, _tooltip_text);
        
        node_a.x += node_a.w;
        node_ix++;
    }
}

void draw_ui_profiler(UI_Element *e, Profiler *prof, Input_Manager *input, Graphics *gfx)
{
    Assert(e->type == UI_PROFILER);
    auto *ui_prof = &e->profiler;
    
    _TRANSLUCENT_UI_();

    int sel0 = ui_prof->selection_start;
    int sel1 = ui_prof->selection_end;
    Assert(sel1 >= sel0);

    bool do_draw_tooltip = false;
    String tooltip_text  = EMPTY_STRING;

    Rect a = ui_prof->a;
    Rect node_a, frame_a, zoom_a, all_a;
    get_profiler_rects(a, &node_a, &frame_a, &zoom_a, &all_a);
    
    draw_rect(a, { 0, .1, .2, .8 }, gfx);

    auto *frame = &prof->frames[ui_prof->selected_frame];
    
    // SELECTED NODE //
    if(ui_prof->selected_node >= 0 && ui_prof->selected_node < frame->num_nodes)
    {
        auto *node = &frame->nodes[ui_prof->selected_node];
        Rect aa = bottom_of(node_a, node_a.h / max_depth_of_profiler_node(node));
        draw_profiler_node(node, 0, aa, input, gfx, &do_draw_tooltip, &tooltip_text);
    }
    
    // SELECTED FRAME //
    draw_rect(frame_a, { 0, .1, .2, .8 }, gfx);
    draw_profile_frame(frame, frame_a, input, gfx, &do_draw_tooltip, &tooltip_text);

    // RANGE SELECTION //
    Rect selection_a = profiler_selection_rect(sel0, sel1, all_a, prof);
    draw_rect(selection_a, { .4, .6, .8, .4 }, gfx);

    // FRAME SELECTION //
    float zoom_frame_w = zoom_a.w / (sel1 - sel0);
    Rect selected_frame_a  = left_of(zoom_a, zoom_frame_w);
    selected_frame_a.x    += zoom_frame_w * (ui_prof->selected_frame - sel0);
    draw_rect(selected_frame_a, { .4, .6, .8, .4 }, gfx);

    //--
    draw_profile_range(sel0, sel1, zoom_a, prof, gfx);
    draw_profile_range(0, ARRLEN(prof->frames), all_a, prof, gfx);


    if(do_draw_tooltip)
    {
        Font_ID font = FONT_TITLE;
        Font_Size fs = FS_12;
        
        v2 text_s = string_size(tooltip_text, fs, font, gfx);
        Rect tooltip_a = { input->mouse.p, text_s + V2_XY * 4 };
        tooltip_a.p.x += 16;
        tooltip_a.p.y -= tooltip_a.h + 4;
        
        draw_rect(tooltip_a, { 0, 0, 0, 0.8 }, gfx);
        draw_string_in_rect_centered(tooltip_text, tooltip_a, FS_12, FONT_TITLE, C_WHITE, gfx);
    }
}
