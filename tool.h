
enum Tool_ID {
    TOOL_NONE,
    
    TOOL_ENTITY_MENU,
    TOOL_PLANTING,

#if DEVELOPER
    TOOL_DEV_ROOM_EDITOR,
#endif
};



struct Entity_Menu {
    Entity_ID entity;
    v2 p;
};

struct Planting_Tool {
    Entity_ID seed_container;

    Seed_Type seed_type_cache; // Cached for drawing. Updated in tool update or something.

    bool waiting_for_pickup_enqueue;
};




enum Tool_Stay_Open_Signal
{
    TOOL_STAY_OPEN,
    TOOL_CLOSE_ANIMATED,
    TOOL_CLOSE_INSTANTLY
};
