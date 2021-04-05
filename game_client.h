
/*
  Client Game
*/

#define Num_Entities(Room_Pointer) ((Room_Pointer)->entities.n)
#define Entities(Room_Pointer) ((Room_Pointer)->entities.e)

#define Entity_Exists(Entity_Pointer) (true)

namespace Client_Game
{
    struct Entity: public S__Entity
    {
        bool is_preview; // We use an entity with this set to true while we wait for a ROOM_UPDATE after we've sent a request to place an item in the room.

        union {
            struct {
                bool is_me;
                Player_State state_before_action_in_queue[ARRLEN(S__Entity::player_e.action_queue)+1]; // Last one is AFTER last action.
            } player_local;

            struct {
                struct {
                    s8 selected_square_ix_plus_one;
                } chess;                
            } item_local;
        };

        bool exists_on_server; // Used on RCB_ROOM_UPDATE to know which entities to remove.
    };
 
    struct Room: public S__Room
    {
        double time_offset; // To get the current World_Time, do <system time> + time_offset.

        Array<Entity, ALLOC_MALLOC> entities;

        // @Cleanup
        // This UI state stuff maybe shouldn't be in the Room struct.
        // If we have multiple rooms, or multiple world views,
        // we would want to have multiple sets of these.       -EH, 2021-03-10
        Entity_ID selected_entity;
        double action_menu_open_t;  // NOTE: Action menu is open if action_menu_open_t > action_menu_close_t.
        double action_menu_close_t;
        Entity_ID action_menu_entity; // NOTE: Needs to be valid during closing animation.
        v2 action_menu_p;
        
        v3      placement_p; // If we for example have a selected inventory item, this is where we would try to put it.
        Quat    placement_q;
        bool    placing_held_item;
        //-------------
        
        bool static_geometry_up_to_date;
    };
    
    void clear_and_reset(Room *room) {
        clear(static_cast<S__Room *>(room));

        for(int i = 0; i < room->entities.n; i++) {
            clear(&room->entities[i]);
        }
        
        room->entities.n = 0;
        room->static_geometry_up_to_date = false;

        Zero(*room);
    }
};
