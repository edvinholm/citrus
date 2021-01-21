
/*
  Server Game
*/

namespace Server_Game
{
    struct Entity
    {
        S__Entity shared; // @Jai: Using
    };
    
    struct Room {
        double t;

        int num_entities;
        Entity entities[MAX_ENTITIES_PER_ROOM];
        
        // @Temporary @NoRelease
        double randomize_cooldown;
        bool did_change;
        // --

        S__Room shared; // @Jai: Using
    };
};
