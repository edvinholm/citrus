
struct Recipe_Component
{
    bool is_liquid;
    union {
        struct {
            Item_Type_ID type;
        } item;
        
        struct {
            Liquid_Type   type;
            Liquid_Amount amount;
        } liquid;
    };
};

struct Recipe
{
    Recipe_Component inputs[MAX_RECIPE_INPUTS];
    u8 num_inputs;

    Liquid_Container output;
};
static_assert(MAX_RECIPE_INPUTS <= U8_MAX); // Because Recipe.num_inputs is a u8


Recipe recipes[3];
static_assert(ARRLEN(recipes) <= S32_MAX); // Because Recipe_ID is s32

Recipe_Component liquid_recipe_component(Liquid_Type type, Liquid_Amount amount)
{
    Recipe_Component component = {0};
    component.is_liquid = true;
    component.liquid.type = type;
    component.liquid.amount = amount;
    return component;
}

Recipe_Component item_recipe_component(Item_Type_ID type)
{
    Recipe_Component component = {0};
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
    { // [PLANT, PLANT, PLANT] -> WATER
        auto &r = recipes[ix++];
        r.inputs[r.num_inputs++] = item_recipe_component(ITEM_PLANT);
        r.inputs[r.num_inputs++] = item_recipe_component(ITEM_PLANT);
        r.inputs[r.num_inputs++] = item_recipe_component(ITEM_PLANT);
        Assert(r.num_inputs < ARRLEN(r.inputs));
        r.output.liquid = water();
        r.output.amount = 15;
    }

    Assert(ix < ARRLEN(recipes));
    { // [WATER] -> WATER
        auto &r = recipes[ix++];
        r.inputs[r.num_inputs++] = liquid_recipe_component(LQ_WATER, 10);
        Assert(r.num_inputs < ARRLEN(r.inputs));
        r.output.liquid = water();
        r.output.amount = 10;
    }
    
    Assert(ix < ARRLEN(recipes));
    { // [WATER, FRUIT] -> YEAST_WATER
        auto &r = recipes[ix++];
        r.inputs[r.num_inputs++] = liquid_recipe_component(LQ_WATER, 9);
        r.inputs[r.num_inputs++] = item_recipe_component(ITEM_FRUIT);
        // @Norelease: Add sugar.
        Assert(r.num_inputs < ARRLEN(r.inputs));
        r.output.liquid = yeast_water(10, 190);
        r.output.amount = 10;
    }
    
}
