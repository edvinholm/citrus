
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
                Player_State state_after_action_in_queue[ARRLEN(S__Entity::player_e.action_queue)];
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
        Entity_ID selected_entity;
        
        v3        placement_tp; // If we for example have a selected inventory item, this is where we would try to put it.
        Entity_ID placement_surface_entity;
        bool      placing_held_item;
        
        
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
