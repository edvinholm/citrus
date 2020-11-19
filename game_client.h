
/*
  Client Game
*/

namespace Client_Game
{
    struct Room {
        S__Room shared;
    };
    void reset(Room *room) {
        reset(&room->shared);
    }

    struct Game
    {
        Room room;
    };
};
