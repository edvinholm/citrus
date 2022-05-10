

struct Pending_Player_Action: public Player_Action
{
    union {
        bool is_pick_up_for_planting;
    };
};


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
    
    struct Pending_RS_Operation
    {
        RSB_Packet_Type rsb_packet_type;
        u32             rsb_packet_id;
        
        bool result_received;
        bool success; // Only valid if result_received.
        RCB_Packet_Result_Payload result_payload;

#if DEVELOPER
        double creation_t;
#endif
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
        
        v3      placement_p; // If we for example have a selected inventory item, this is where we would try to put it.
        Quat    placement_q;
        bool    placing_held_item;

        Entity_ID action_menu_entity;
        v2 action_menu_p;
        double action_menu_open_t;
        double action_menu_close_t;
        //-------------

        Pending_Player_Action pending_actions[ARRLEN(S__Entity::player_e.action_queue)];
        u32                   pending_action_rsb_packet_ids[ARRLEN(pending_actions)];
        int num_pending_actions;
        
        bool static_geometry_up_to_date;

        Array<Pending_RS_Operation, ALLOC_MALLOC> pending_rs_operations;
    };
    
    void clear_and_reset(Room *room) {
        clear(static_cast<S__Room *>(room));

        for(int i = 0; i < room->entities.n; i++) {
            clear(&room->entities[i]);
        }
        
        room->entities.n = 0;
        room->static_geometry_up_to_date = false;

        clear(&room->pending_rs_operations);

        Zero(*room);
    }
};
