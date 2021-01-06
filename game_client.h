
/*
  Client Game
*/

namespace Client_Game
{
    struct Room {
        S__Room shared;

        bool static_geometry_up_to_date;
    };
    void reset(Room *room) {
        reset(&room->shared);
    }

    struct Game
    {
        Room room;
    };
};
