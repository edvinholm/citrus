
struct Recipe_Input
{
    bool is_liquid;
    union {
        struct {
            Item_Type_ID type;
        } item;
        
        struct {
            Liquid_Type      type;
            Substance_Amount amount;
        } liquid;
    };
};

struct Recipe
{
    Recipe_Input inputs[MAX_RECIPE_INPUTS];
    u8 num_inputs;

    Substance_Container outputs[MAX_RECIPE_OUTPUTS];
    u8 num_outputs;
};
static_assert(MAX_RECIPE_INPUTS  <= U8_MAX); // Because Recipe.num_inputs is a u8
static_assert(MAX_RECIPE_OUTPUTS <= U8_MAX); // Because Recipe.num_outputs is a u8


Recipe recipes[4];
static_assert(ARRLEN(recipes) <= S32_MAX); // Because Recipe_ID is s32

Recipe_Input liquid_recipe_input(Liquid_Type type, Substance_Amount amount)
{
    Recipe_Input component = {0};
    component.is_liquid = true;
    component.liquid.type = type;
    component.liquid.amount = amount;
    return component;
}

Recipe_Input item_recipe_input(Item_Type_ID type)
{
    Recipe_Input component = {0};
    component.item.type = type;
    return component;
}

// @Cleanup: Can't do this for all liquids.... Just for now. Also, shouldn't be in this file.
Liquid water() {
    Liquid lq = {0};
    lq.type = LQ_WATER;
    return lq;
}

Liquid yeast_water(Liquid_Fraction yeast, Liquid_Fraction nutrition) {
    Liquid lq = {0};
    lq.type = LQ_YEAST_WATER;
    lq.yeast_water.yeast     = yeast;
    lq.yeast_water.nutrition = nutrition;
    return lq;
}

/* IMPORTANT: Don't make recipes that takes less than, say, 1 second.
              At least until we have better ways of "simulating" on the client.
              We don't want a lot of ROOM_UPDATEs.  -EH, 2021-03-04
 */

void init_recipes()
{
    int ix = 0;
    
    Assert(ix < ARRLEN(recipes));
    { // [WATER] -> WATER
        auto &r = recipes[ix++];
        r.inputs[r.num_inputs++] = liquid_recipe_input(LQ_WATER, 10);
        Assert(r.num_inputs < ARRLEN(r.inputs));
        
        r.outputs[0].substance.form = SUBST_LIQUID;
        r.outputs[0].substance.liquid = water();
        r.outputs[0].amount = 10;
        r.num_outputs = 1;
        Assert(r.num_outputs < ARRLEN(r.outputs));
    }
    
    Assert(ix < ARRLEN(recipes));
    { // [WATER, FRUIT] -> YEAST_WATER
        auto &r = recipes[ix++];
        r.inputs[r.num_inputs++] = liquid_recipe_input(LQ_WATER, 9);
        r.inputs[r.num_inputs++] = item_recipe_input(ITEM_FRUIT);
        // @Norelease: Add sugar.
        Assert(r.num_inputs < ARRLEN(r.inputs));
        
        r.outputs[0].substance.form = SUBST_LIQUID;
        r.outputs[0].substance.liquid = yeast_water(10, 190);
        r.outputs[0].amount = 10;
        r.num_outputs = 1;
        Assert(r.num_outputs < ARRLEN(r.outputs));
    }


    
    Assert(ix < ARRLEN(recipes));
    { // [YEAST_WATER] -> YEAST, WATER
        auto &r = recipes[ix++];
        r.inputs[r.num_inputs++] = liquid_recipe_input(LQ_YEAST_WATER, 10);
        Assert(r.num_inputs < ARRLEN(r.inputs));

        static_assert(ARRLEN(r.inputs) >= 2);
        
        r.outputs[0].substance.form        = SUBST_NUGGET;
        r.outputs[0].substance.nugget.type = NUGGET_YEAST;
        r.outputs[0].amount = 10; // @Norelease: This should depend on the amount of yeast in the water.

        r.outputs[1].substance.form = SUBST_LIQUID;
        r.outputs[1].substance.liquid = water();
        r.outputs[1].amount = 10; // @Norelease: This should ALSO(!) depend on the amount of yeast in the water.
        
        r.num_outputs = 2;
    }
    
}
