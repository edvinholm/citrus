

enum UI_Dock_Split_Mode
{
    UI_DOCK_SPLIT_NONE = 0,
    UI_DOCK_SPLIT_HORIZONTAL = 1,
    UI_DOCK_SPLIT_VERTICAL = 2,
    UI_DOCK_SPLIT_TABBED = 3
};

struct UI_Dock_Section
{
    UI_Dock_Split_Mode split;
    union {
        struct {
            struct {
                u32 split_percentage; // left/bottom sub section is this many percent of this section's width/height
                float split_slider_mouse_drag_offset; // This is from the border between the sub sections
            };
            struct {
                u32 visible_tab;
            };
            u32 sub_sections[8];  // index in UI_Dock.sections
        };
        View view;
    };
};


struct UI_Dock
{
    // Root section is sections[0]
    Array<UI_Dock_Section, ALLOC_MALLOC> sections;
    Array<u32, ALLOC_MALLOC> free_section_slots; // We don't remove sections, because we have indices pointing to them.

    bool unsaved_changes; // This is true if there is changes that has not yet been saved to disk.
    double save_t;        // Last time we saved the layout to disk.
};

void clear(UI_Dock *dock)
{
    for(int i = 0; i < dock->sections.n; i++) {
        auto *it = &dock->sections[i];
        if(it->split != UI_DOCK_SPLIT_NONE) continue;
        clear(&it->view);
    }
    clear(&dock->sections);
    clear(&dock->free_section_slots);
}
