
/*
  Client Game
*/

namespace Client_Game
{
    
    struct Entity
    {
        S__Entity shared; // @Jai: Using
        bool is_preview; // We use an entity with this set to true while we wait for a ROOM_UPDATE after we've sent a request to place an item in the room.
    };
 
    struct Room
    {
        S__Room shared; // @Jai: Using

        double time_offset; // To get the current World_Time, do <system time> + time_offset.

        bool static_geometry_up_to_date;

        Array<Entity, ALLOC_GAME> entities;
    };
    void reset(Room *room) {
        reset(&room->shared);
        room->entities.n = 0;
        room->static_geometry_up_to_date = false;
    }

    struct Game
    {
        Room room;
    };
};
