
/*
  Server Game
*/

#define Num_Entities(Room_Pointer) ((Room_Pointer)->num_entities)
#define Entities(Room_Pointer) ((Room_Pointer)->entities)

#define Entity_Exists(Entity_Pointer) (!(Entity_Pointer)->scheduled_for_destruction)

namespace Server_Game
{
    struct Entity: public S__Entity
    {
        bool scheduled_for_destruction;
    };

    struct Room: public S__Room {
        
        Entity_ID next_entity_id_minus_one;
        int num_entities;
        Entity entities[MAX_ENTITIES_PER_ROOM];

        Player_Action_ID next_player_action_id;
        
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
