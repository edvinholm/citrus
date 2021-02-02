
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

        Walk_Mask walk_map[room_size_x * room_size_y];
        
        // @Temporary @NoRelease
        double randomize_cooldown;
        bool did_change;
        // --
    };

    void clear_and_reset(Room *room) {
        clear(static_cast<S__Room *>(room));
        
        for(int i = 0; i < room->num_entities; i++) {
            clear(&room->entities[i]);
        }

        Zero(*room);
    }
};
