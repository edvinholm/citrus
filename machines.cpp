

// NOTE: This just begins doing the recipe. Its possibility should have been checked beforehand.
// NOTE: Indices of inputs should map to indices of recipe->inputs.
void begin_recipe(Recipe *recipe, Entity **inputs, int num_inputs, Entity *e, Room *room, bool entity_items_already_updated = false)
{
    if(!entity_items_already_updated)
        update_entity_item(e, room->t);
    
    Assert(e->type == ENTITY_ITEM);
    auto *item = &e->item_e.item;

    Assert(item->type == ITEM_BLENDER);
    auto *blender = &e->item_e.blender;
    
    blender->t_on_recipe_begin = room->t;
    blender->recipe_duration   = 2; // @Hardcoded

    auto t0 = room->t;
    auto t1 = room->t + blender->recipe_duration;

    
    auto lc0 = item->liquid_container;
    auto current_lq = liquid_type_of_container(&lc0);
    auto lc1 = lc0;

    if(current_lq == LQ_NONE_OR_NUM) { // This is to avoid floating point weirdness. If we decide that the container is empty, but amount is actually like 0.001, we might get extra liquid over time...(?) Or am i just overthinking this?  -EH, 2021-03-03
        lc1 = recipe->output;
        lc0.liquid = lc1.liquid;
    } else {
        Assert(liquid_type_of_container(&lc1) != LQ_NONE_OR_NUM);
        Assert(can_blend(&lc1, &recipe->output));

        // NOTE: For now, we don't simulate the properties of the recipe output liquid
        //       during the progress of the recipe.
        //       The output's properties are what they are when the recipe starts.

        simulate_liquid_properties(&lc1, t1 - t0);
        
        lc1 = blend(&lc1, &recipe->output);
    }

    blender->recipe_inputs.n = num_inputs;
    for(int i = 0; i < num_inputs; i++) {
        auto *input_entity = inputs[i];
        Assert(input_entity->type == ENTITY_ITEM);
 
        if(!entity_items_already_updated) // @Jai: @Speed: #bake a version where we do the updates, and one where we don't.
            update_entity_item(input_entity, room->t);
        
        auto *item = &input_entity->item_e.item;
        if(recipe->inputs[i].is_liquid) {
            Assert(item_types[item->type].flags & ITEM_IS_LQ_CONTAINER);
            
            input_entity->item_e.lc_t0 = t0;
            input_entity->item_e.lc_t1 = t1;
            input_entity->item_e.lc0 = item->liquid_container;

            auto lc1 = item->liquid_container;
            lc1.amount -= recipe->inputs[i].liquid.amount;// @Hack @Norelease: Recipe_Component should have a required amount thing.
            Assert(lc1.amount >= 0);
            input_entity->item_e.lc1 = lc1;
        }

        lock_item_entity(input_entity, e->id);
        blender->recipe_inputs[i] = input_entity->id;
    }
    

    e->item_e.lc_t0 = t0;
    e->item_e.lc_t1 = t1;
    e->item_e.lc0 = lc0;
    e->item_e.lc1 = lc1;
    
    room->did_change = true;
}


bool is_machine(Entity *e)
{
    if(e->type != ENTITY_ITEM) return false;
    if(e->item_e.item.type != ITEM_BLENDER) return false;
    return true;
}

bool machine_is_doing_recipe(Entity *e, double world_t)
{
    Assert(is_machine(e));
    switch(e->item_e.item.type) {
        
        case ITEM_BLENDER: {
            auto *blender = &e->item_e.blender;
            if(blender->t_on_recipe_begin + blender->recipe_duration >= world_t) return true;
        } break;

        default: Assert(false); break;
    }

    return false;
}

bool can_be_used_as_recipe_input(Entity *e, double world_t)
{
    if(e->type != ENTITY_ITEM) return false;
    if(e->item_e.locked_by != NO_ENTITY) return false;
    if(is_machine(e)) {
        if(machine_is_doing_recipe(e, world_t)) return false;
    }

    return true;
}

void maybe_begin_machine_recipe(Entity *e, Room *room)
{
    Assert(is_machine(e));
    update_entity_item(e, room->t);
    auto *item = &e->item_e.item;

    if(machine_is_doing_recipe(e, room->t)) return;
    if(e->item_e.locked_by != NO_ENTITY) return;

    Array<Entity *, ALLOC_TMP> supported_entities = {0};
    find_supported_entities(e, room, &supported_entities);

    switch(item->type) {
        case ITEM_BLENDER: {
            auto *blender = &e->item_e.blender;
            if(blender->t_on_recipe_begin + blender->recipe_duration > room->t) return; // We're already doing a recipe.
        } break;
            
        default: return;
    }      
    
    // UPDATE ITEMS OF ALL SUPPORTED ENTITIES //
    for(int i = 0; i < supported_entities.n; i++) {
        auto *sup = supported_entities[i];
        if(sup->type != ENTITY_ITEM) continue;
        update_entity_item(sup, room->t);
    }

    // FIND A MATCHING RECIPE //
    Entity *found_inputs[ARRLEN(Recipe::inputs)] = { 0 };
    int num_found_inputs = 0;
    
    Recipe *found_recipe = NULL;
    for(int r = 0; r < ARRLEN(recipes); r++) {
        auto *rec = &recipes[r];
        if(rec->num_inputs != supported_entities.n) continue;

        num_found_inputs = 0;
        memset(found_inputs, 0, sizeof(found_inputs));
        
        for(int i = 0; i < rec->num_inputs; i++) {

            Recipe_Component &input = rec->inputs[i];
            if (found_inputs[i] != NULL) continue;
            
            for(int k = 0; k < supported_entities.n; k++) {
                auto *sup = supported_entities[k];
                if(sup->type != ENTITY_ITEM) continue;

                if(!can_be_used_as_recipe_input(sup, room->t)) continue;

                if(input.is_liquid) {
                    if(!(item_types[sup->item_e.item.type].flags & ITEM_IS_LQ_CONTAINER)) continue;
                    auto *lc = &sup->item_e.item.liquid_container;
                    if(lc->liquid.type != input.liquid.type) continue;
                    if(lc->amount < input.liquid.amount) continue;
                } else {
                    if(sup->item_e.item.type != input.item.type) continue;
                }

                bool already_used = false;
                for(int j = 0; j < rec->num_inputs; j++) {
                    if(found_inputs[j] == sup) {
                        already_used = true;
                        break;
                    }
                }

                if(already_used) continue;

                Assert(num_found_inputs < rec->num_inputs);
                Assert(num_found_inputs < ARRLEN(found_inputs));
                Assert(found_inputs[i] == NULL);
                found_inputs[i] = sup;
                num_found_inputs++;
                break;
            }
        }
        
        if(num_found_inputs == rec->num_inputs) {
            Assert(num_found_inputs == supported_entities.n);
            found_recipe = rec;
            break;
        }
    }

    if(found_recipe) {

        // TODO @Cleanup: We should have some standard way to add liquid / check if adding liquid to a Liquid_Container is possible.        
        auto *lc = &item->liquid_container;
        auto capacity = liquid_container_capacity(item);
        
        Assert(item_types[item->type].flags & ITEM_IS_LQ_CONTAINER);
        
        // NOTE: This is not the same as (capacity - lc->amount >= found_recipe->output.amount).
        //       The Assert check below is about if this recipe is suitable at all for this machine.
        //       The condition above is "can we do the recipe right now given what we have in our container?"
        Assert(capacity >= found_recipe->output.amount); // Should have checked this earlier, when we looked through all available recipes. If we do it now, it's too late -- there might be a better recipe that meets this requirement.

        auto current_lq = liquid_type_of_container(lc);
        if(can_blend(lc, &found_recipe->output))
        {
            if(capacity - lc->amount >= found_recipe->output.amount) {
                begin_recipe(found_recipe, found_inputs, num_found_inputs, e, room, true);
            }
        }        
    }
    
}


void update_blender(Entity *e, Room *room)
{
    Assert(e->type == ENTITY_ITEM);
    Assert(e->item_e.item.type == ITEM_BLENDER);
    
    auto *blender = &e->item_e.blender;
    if(blender->t_on_recipe_begin <= 0) return;

    auto time_since_start = room->t - blender->t_on_recipe_begin;
    if(doubles_equal(time_since_start, blender->recipe_duration)) {

        // DESTROY/UNLOCK INPUTS //
        for(int i = 0; i < blender->recipe_inputs.n; i++) {
            auto *input_entity = find_entity(blender->recipe_inputs[i], room);
            Assert(input_entity); // Because we should have locked it.

            if(item_types[input_entity->item_e.item.type].flags & ITEM_IS_LQ_CONTAINER) {
                unlock_item_entity(input_entity, e->id, room);
            } else {
                schedule_for_destruction(input_entity, room, e->id);
            }
        }

        // MAKE SURE WE REACH lc1! //
        e->item_e.lc0 = e->item_e.lc1;

        // SET RECIPE TO NONE //
        blender->t_on_recipe_begin = 0;

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
