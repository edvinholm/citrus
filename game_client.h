
/*
  Client Game
*/

namespace Client_Game
{
    
    struct Entity: public S__Entity
    {
        bool is_preview; // We use an entity with this set to true while we wait for a ROOM_UPDATE after we've sent a request to place an item in the room.

        union {
            struct {
                bool is_me;
            } player_local;
        };

        bool exists_on_server; // Used on RCB_ROOM_UPDATE to know which entities to remove.
    };
 
    struct Room: public S__Room
    {
        double time_offset; // To get the current World_Time, do <system time> + time_offset.

        Array<Entity, ALLOC_GAME> entities;
        Entity_ID selected_entity;
        
        bool static_geometry_up_to_date;
    };
    void reset(Room *room) {
        reset(static_cast<S__Room *>(room));
        room->entities.n = 0;
        room->static_geometry_up_to_date = false;
    }

    struct Game
    {
        Room room;
    };
};
