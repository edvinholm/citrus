
/*
  Server Game
*/

namespace Server_Game
{
    struct Entity: public S__Entity
    {
    };
    
    struct Room: public S__Room {
        Entity_ID next_entity_id_minus_one;
        int num_entities;
        Entity entities[MAX_ENTITIES_PER_ROOM];
        
        // @Temporary @NoRelease
        double randomize_cooldown;
        bool did_change;
        // --
    };
};
