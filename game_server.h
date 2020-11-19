
/*
  Server Game
*/

namespace Server_Game
{
    struct Room {
        double t;
        
        // @Temporary @NoRelease
        double randomize_cooldown;
        bool did_change;
        // --

        S__Room shared;
    };
};
