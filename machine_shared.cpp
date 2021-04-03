
bool is_machine(Entity *e, Machine **_machine = NULL)
{
    if(e->type != ENTITY_ITEM) return false;
    
    switch(e->item_e.item.type) {
        case ITEM_BLENDER: {
            if(_machine) *_machine = &e->item_e.blender.machine;
            return true;
        } break;
            
        case ITEM_FILTER_PRESS: {
            if(_machine) *_machine = &e->item_e.filter_press.machine;
            return true;
        } break;
            
        case ITEM_GRINDER: {
            if(_machine) *_machine = &e->item_e.grinder.machine;
            return true;
        } break;
    }
    
    return false;
}

bool machine_is_doing_recipe(Machine *machine, double world_t)
{
    return (machine->t_on_recipe_begin > 0 &&
            machine->t_on_recipe_begin + machine->recipe_duration >= world_t);
}

bool can_be_used_as_recipe_input(Entity *e, double world_t)
{
    if(e->type != ENTITY_ITEM) return false;
    if(e->item_e.locked_by != NO_ENTITY) return false;
    
    Machine *machine;
    if(is_machine(e, &machine)) {
        if(machine_is_doing_recipe(machine, world_t)) return false;
    }

    return true;
}
