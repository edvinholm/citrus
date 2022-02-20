

enum UI_Dock_Split_Mode
{
    UI_DOCK_SPLIT_NONE = 0,
    UI_DOCK_SPLIT_HORIZONTAL,
    UI_DOCK_SPLIT_VERTICAL
};

struct UI_Dock_Section
{
    UI_Dock_Split_Mode split;
    union {
        struct {
            int split_percentage; // left/bottom sub section is this many percent of this section's width/height
            int sub_sections[2];  // index in UI_Dock.sections
            float split_slider_mouse_drag_offset; // This is from the border between the sub sections
        };
        View view;
    };
};


struct UI_Dock
{
    // Root section is sections[0]
    Array<UI_Dock_Section, ALLOC_MALLOC> sections;
    Array<int, ALLOC_MALLOC> free_section_slots; // We don't remove sections, because we have indices pointing to them.
};
