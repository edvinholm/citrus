
/*
  Client Game
*/

namespace Client_Game
{
    
    struct Entity
    {
        S__Entity shared; // @Jai: Using
    };
 
    struct Room
    {
        S__Room shared; // @Jai: Using

        bool static_geometry_up_to_date;
        
        int num_entities;
        Entity entities[MAX_ENTITIES_PER_ROOM];
    };
    void reset(Room *room) {
        reset(&room->shared);
    }

    struct Game
    {
        Room room;
    };
};
