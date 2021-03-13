
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
    }
    
    return false;
}

bool machine_is_doing_recipe(Machine *machine, double world_t)
{
    return (machine->t_on_recipe_begin + machine->recipe_duration >= world_t);
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


// NOTE: This just begins doing the recipe. Its possibility should have been checked beforehand.
// NOTE: Indices of inputs should map to indices of recipe->inputs.
void begin_recipe(Recipe *recipe, Entity **inputs, int num_inputs, Entity **outputs, int num_outputs, Entity *e, Room *room, bool entity_items_already_updated = false)
{
    Machine *machine;
    if(!is_machine(e, &machine)) {
        Assert(false);
        return;
    }
    
    if(!entity_items_already_updated) {
        update_entity_item(e, room->t);
    }


    Assert(e->type == ENTITY_ITEM);
    auto *item = &e->item_e.item;

    machine->t_on_recipe_begin = room->t;
    machine->recipe_duration   = 2; // @Hardcoded
    
    auto t0 = room->t;
    auto t1 = room->t + machine->recipe_duration;


    Assert(num_outputs == recipe->num_outputs);
    machine->recipe_outputs.n = num_outputs;
    for(int i = 0; i < recipe->num_outputs; i++)
    {
        auto *recipe_output = &recipe->outputs[i];
        
        auto *output_container = outputs[i];
        Assert(output_container->type == ENTITY_ITEM);
         
        if(!entity_items_already_updated) // @Jai: @Speed: #bake a version where we do the updates, and one where we don't.
            update_entity_item(output_container, room->t);
        
        Item *output_container_item = &output_container->item_e.item;

        switch(recipe->outputs[i].form)
        {
            case FORM_LIQUID: {
                Assert(item_types[output_container_item->type].container_form == FORM_LIQUID);
            
                auto lc0 = output_container_item->liquid_container;
                auto current_lq = liquid_type_of_container(&lc0);
                auto lc1 = lc0;

                // NOTE: For now, we don't simulate the properties of the recipe output liquid
                //       during the progress of the recipe.
                //       The output's properties are what they are when the recipe starts.

                simulate_liquid_properties(&lc1, t1 - t0);
                lc1 = blend(&lc1, &recipe_output->liquid);

            
                auto *c = &output_container->item_e.container;
                auto *l = &c->liquid;

                c->t0 = t0;
                c->t1 = t1;
                l->c0 = lc0;
                l->c1 = lc1;
            
            } break;
            
            case FORM_NUGGET: {
                Assert(item_types[output_container_item->type].container_form == FORM_NUGGET);
            
                auto nc0 = output_container_item->nugget_container;
                auto current_lq = nugget_type_of_container(&nc0);
                auto nc1 = nc0;

                nc1 = blend(&nc1, &recipe_output->nugget);
            
                auto *c = &output_container->item_e.container;
                auto *n = &c->nugget;

                c->t0 = t0;
                c->t1 = t1;
                n->c0 = nc0;
                n->c1 = nc1;
            
            } break;

            default: Assert(false); return;
        }
    
        lock_item_entity(output_container, e->id);
        machine->recipe_outputs[i] = output_container->id;
    }
    
    machine->recipe_inputs.n = num_inputs;
    for(int i = 0; i < num_inputs; i++) {
        auto *input_entity = inputs[i];
        Assert(input_entity->type == ENTITY_ITEM);
 
        if(!entity_items_already_updated) // @Jai: @Speed: #bake a version where we do the updates, and one where we don't.
            update_entity_item(input_entity, room->t);
        
        auto *item = &input_entity->item_e.item;
        if(recipe->inputs[i].is_liquid) {
            Assert(item_types[item->type].container_form == FORM_LIQUID);

            auto *c = &input_entity->item_e.container;
            auto *l = &c->liquid;
            
            c->t0 = t0;
            c->t1 = t1;
            l->c0 = item->liquid_container;

            auto lc1 = item->liquid_container;
            lc1.amount -= recipe->inputs[i].liquid.amount;// @Hack @Norelease: Recipe_Component should have a required amount thing.
            Assert(lc1.amount >= 0);
            l->c1 = lc1;
        }

        lock_item_entity(input_entity, e->id);
        machine->recipe_inputs[i] = input_entity->id;
    }

    
    room->did_change = true;
}


bool can_be_used_as_recipe_output_container(Entity *e, double world_t)
{
    if(e->type != ENTITY_ITEM) return false;
    if(e->item_e.locked_by != NO_ENTITY) return false;

    Machine *machine;
    if(is_machine(e, &machine)) {
        if(machine_is_doing_recipe(machine, world_t)) return false;
    }

    return true;
}

bool recipe_suitable_for_machine(Recipe *rec, Entity *machine_entity)
{
    Assert(is_machine(machine_entity));

    switch(machine_entity->item_e.item.type) {
        case ITEM_BLENDER: {
            for(int i = 0; i < rec->num_outputs; i++)
                if(rec->outputs[i].form != FORM_LIQUID) return false;
        } break;

        case ITEM_FILTER_PRESS: {

            if(rec->num_inputs != 1) return false;
            if(!rec->inputs[0].is_liquid) return false;
            
            if(rec->num_outputs != 2) return false;

            bool found_nugget = false;
            bool found_liquid = false;
            for(int i = 0; i < rec->num_outputs; i++) {
                if(rec->outputs[i].form == FORM_NUGGET) found_nugget = true;
                if(rec->outputs[i].form == FORM_LIQUID) found_liquid = true;
            }

            if(!found_nugget || !found_liquid) return false;
            
        } break;
    }

    return true;
}

// @Jai: #bake this for different machine types?
void maybe_begin_machine_recipe(Entity *e, Room *room)
{
    Machine *machine;
    if(!is_machine(e, &machine)) {
        Assert(false);
        return;
    }

    update_entity_item(e, room->t);
    auto *item = &e->item_e.item;

    if(machine_is_doing_recipe(machine, room->t)) return;
    if(e->item_e.locked_by != NO_ENTITY) return;

    Array<Support, ALLOC_TMP> supports = {0};
    find_given_supports(e, room->t, room, &supports);

    int num_supported_inputs  = 0;
    int num_supported_outputs = 0;
    
    // UPDATE ITEMS OF ALL SUPPORTED ENTITIES //
    for(int i = 0; i < supports.n; i++) {
        auto &sup = supports[i];

        if(!sup.is_first_support_of_supported) continue;
        
        if(sup.supported->type != ENTITY_ITEM) continue;
        update_entity_item(sup.supported, room->t);

        switch(sup.surface_type) {
            case SURF_TYPE_MACHINE_INPUT:  num_supported_inputs++;  break;
            case SURF_TYPE_MACHINE_OUTPUT: {
                num_supported_outputs++;
            } break;
        }        
    }

    
    // FIND A MATCHING RECIPE //
    Entity *found_inputs[ARRLEN(Recipe::inputs)] = { 0 };
    int num_found_inputs = 0;
    
    Entity *found_outputs[ARRLEN(Recipe::outputs)] = { 0 };
    int num_found_outputs = 0;
    
    Recipe *found_recipe = NULL;
    for(int r = 0; r < ARRLEN(recipes); r++) {
        auto *rec = &recipes[r];

        if(!recipe_suitable_for_machine(rec, e)) continue;
        
        if(rec->num_inputs  != num_supported_inputs)  continue;
        if(rec->num_outputs != num_supported_outputs) continue;

        // FIND OUTPUTS //
        num_found_outputs = 0;
        memset(found_outputs, 0, sizeof(found_outputs));

        for(int i = 0; i < rec->num_outputs; i++) {
            auto &output = rec->outputs[i];
            if (found_outputs[i] != NULL) continue;
                        
            for(int k = 0; k < supports.n; k++) {
                auto &sup = supports[k];
                if(!sup.is_first_support_of_supported) continue;
                
                if(sup.surface_type != SURF_TYPE_MACHINE_OUTPUT) continue;

                auto *supported = sup.supported;
                if(supported->type != ENTITY_ITEM) continue;

                if(!can_be_used_as_recipe_output_container(supported, room->t)) continue;

                switch(output.form) { // @Jai: #complete
                    case FORM_LIQUID: {
                        if(item_types[supported->item_e.item.type].container_form != FORM_LIQUID) continue;
                        
                        auto *lc = &supported->item_e.item.liquid_container;
                        auto capacity = liquid_container_capacity(&supported->item_e.item);
                        if(capacity - lc->amount < output.liquid.amount) continue;
                    } break;

                    case FORM_NUGGET: {
                        if(item_types[supported->item_e.item.type].container_form != FORM_NUGGET) continue;

                        auto *nc = &supported->item_e.item.nugget_container;
                        auto capacity = nugget_container_capacity(&supported->item_e.item);
                        if(capacity - nc->amount < output.nugget.amount) continue;
                    } break;

                    default: Assert(false); break;
                }

                bool already_used = false;
                for(int j = 0; j < rec->num_outputs; j++) {
                    if(found_outputs[j] == supported) {
                        already_used = true;
                        break;
                    }
                }

                if(already_used) continue;

                Assert(num_found_outputs < rec->num_outputs);
                Assert(num_found_outputs < ARRLEN(found_outputs));
                Assert(found_outputs[i] == NULL);
                found_outputs[i] = supported;
                num_found_outputs++;
                break;
            }
        }
        

        // FIND INPUTS //
        num_found_inputs = 0;
        memset(found_inputs, 0, sizeof(found_inputs));
        
        for(int i = 0; i < rec->num_inputs; i++) {

            auto &input = rec->inputs[i];
            if (found_inputs[i] != NULL) continue;
            
            for(int k = 0; k < supports.n; k++) {
                auto &sup = supports[k];
                if(!sup.is_first_support_of_supported) continue;
                
                if(sup.surface_type != SURF_TYPE_MACHINE_INPUT) continue;

                auto *supported = sup.supported;
                if(supported->type != ENTITY_ITEM) continue;

                if(!can_be_used_as_recipe_input(supported, room->t)) continue;

                if(input.is_liquid) {
                    if(item_types[supported->item_e.item.type].container_form != FORM_LIQUID) continue;
                    auto *lc = &supported->item_e.item.liquid_container;
                    if(liquid_type_of_container(lc) != input.liquid.type) continue;
                    if(lc->amount < input.liquid.amount) continue;
                } else {
                    if(supported->item_e.item.type != input.item.type) continue;
                }

                bool already_used = false;
                for(int j = 0; j < rec->num_inputs; j++) {
                    if(found_inputs[j] == supported) {
                        already_used = true;
                        break;
                    }
                }

                if(already_used) continue;

                Assert(num_found_inputs < rec->num_inputs);
                Assert(num_found_inputs < ARRLEN(found_inputs));
                Assert(found_inputs[i] == NULL);
                found_inputs[i] = supported;
                num_found_inputs++;
                break;
            }
        }


        // DID WE FIND OUR RECIPE ? //
        if(num_found_outputs == rec->num_outputs &&
           num_found_inputs == rec->num_inputs)
        {
            Assert(num_found_inputs == num_supported_inputs);
            Assert(num_found_outputs == num_supported_outputs);
            found_recipe = rec;
            break;
        }
    }

    
    if(found_recipe)
    {
        bool can = true;


        // Special thing for filter press: Check that we have the output nugget container on the nugget surface, and the output liquid container on the liquid surface.
        if(item->type == ITEM_FILTER_PRESS) {
            for(int i = 0; i < num_found_outputs; i++) {
                Entity *output_container = found_outputs[i];
                for(int j = 0; j < supports.n; j++) {
                    auto *sup = &supports[j];
                    if(!sup->is_first_support_of_supported) continue;
                    if(sup->supported == output_container) {
                        
                        Item_Form wanted_form = FORM_NONE_OR_NUM;
                        switch(sup->surface_owner_specifics.filter_press_surface_id) {
                            case FILTER_PRESS_SURF_LIQUID_OUTPUT: wanted_form = FORM_LIQUID; break;
                            case FILTER_PRESS_SURF_NUGGET_OUTPUT: wanted_form = FORM_NUGGET; break;
                            default: Assert(false); return;
                        }
                        Assert(wanted_form != FORM_NONE_OR_NUM);

                        Item_Form given_form = item_types[output_container->item_e.item.type].container_form;
                        if(given_form != wanted_form) return;
                    }
                }
            }
        }
        

        for(int i = 0; i < found_recipe->num_outputs; i++)
        {
            auto *recipe_output = &found_recipe->outputs[i];
            
            Assert(found_outputs[i]->type == ENTITY_ITEM);
            auto *output_container_item = &found_outputs[i]->item_e.item;
            
            switch(recipe_output->form) {
                
                case FORM_LIQUID: {
                    Assert(item_types[output_container_item->type].container_form == FORM_LIQUID);
                
                    // TODO @Cleanup: We should have some standard way to add liquid / check if adding liquid to a Liquid_Container is possible.        
                    auto *lc = &output_container_item->liquid_container;
                    auto capacity = liquid_container_capacity(output_container_item);
        
                    // NOTE: This is not the same as (capacity - lc->amount >= found_recipe->output.amount).
                    //       The Assert check below is about if this recipe is suitable at all for this machine.
                    //       The condition above is "can we do the recipe right now given what we have in our container?"
                    Assert(capacity >= recipe_output->liquid.amount); // Should have checked this earlier, when we looked through all available recipes. If we do it now, it's too late -- there might be a better recipe that meets this requirement.

                    if(!can_blend(lc, &recipe_output->liquid)) {
                        can = false;
                        break;
                    }
                    
                    Assert(capacity - lc->amount >= recipe_output->liquid.amount);
                
                } break;

                case FORM_NUGGET: {
                    Assert(item_types[output_container_item->type].container_form == FORM_NUGGET);

                    auto *nc = &output_container_item->nugget_container;
                    auto capacity = nugget_container_capacity(output_container_item);
        
                    Assert(capacity >= recipe_output->nugget.amount);

                    if(!can_blend(nc, &recipe_output->nugget)) {
                        can = false;
                        break;
                    }        
                
                } break;
                
            
                default: Assert(false); break;
            }

            if(!can) break;
        }

        if(can) {
            begin_recipe(found_recipe, found_inputs, num_found_inputs, found_outputs, num_found_outputs, e, room, true);
        }
    }
    
}


void update_machine(Machine *machine, Room *room, Entity *e)
{
#if DEBUG
    {
        Machine *m;
        Assert(is_machine(e, &m) && m == machine);
    }
#endif
    
    if(machine->t_on_recipe_begin <= 0) return;

    auto time_since_start = room->t - machine->t_on_recipe_begin;
    if(doubles_equal(time_since_start, machine->recipe_duration)) {

        // DESTROY/UNLOCK INPUTS //
        for(int i = 0; i < machine->recipe_inputs.n; i++) {
            auto *input_entity = find_entity(machine->recipe_inputs[i], room);
            Assert(input_entity); // Because we should have locked it.

            if(item_types[input_entity->item_e.item.type].container_form == FORM_LIQUID) {
                unlock_item_entity(input_entity, e->id, room);
            } else {
                schedule_for_destruction(input_entity, room, e->id);
            }
        }

        // UNLOCK OUTPUT CONTAINERS //
        for(int i = 0; i < machine->recipe_outputs.n; i++) {
            auto *output_container = find_entity(machine->recipe_outputs[i], room);
            Assert(output_container); // Because we should have locked it.
            unlock_item_entity(output_container, e->id, room);
        }

        // SET RECIPE TO NONE //
        machine->t_on_recipe_begin = 0;

        // @Speed!
        update_supporters_of(e, room); // IMPORTANT that we do this before maybe_begin_machine_recipe,
                                       //           if we want supporters to have a chance to use us as
                                       //           recipe inputs. Right now, I think this is the desired
                                       //           behaviour... -EH, 2021-03-04
        
        // TRY TO FIND A NEW RECIPE TO DO //
        maybe_begin_machine_recipe(e, room);

        room->did_change = true;
    }
}
