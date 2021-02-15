

enum State_Of_Matter: u8
{
    SOLID  = 0x01,
    LIQUID = 0x02,
    GAS    = 0x04
};

struct Substance_Type
{
    const char *name;
    bool can_be_solid;  // We can maybe change these to temperatures in the future...
    bool can_be_liquid;
    bool can_be_gas;
};

Substance_Type substance_types[] = {
    { "Dirt",   true, false, false },
    { "Coffee", false, true, false }
};


enum Item_Flag_: u16
{
    PLACEABLE = 0x0001,
    CONTAINER = 0x0002,
};

typedef u16 Item_Flags;

enum Item_Type_ID
{
    ITEM_PLANT = 1,
    ITEM_FRUIT = 2,
    ITEM_SEED  = 3
};

struct Placeable
{
    u8 sx;
    u8 sy;
    u8 sz;
};

struct Container
{
    State_Of_Matter holdable_states_of_matter;
    float volume; // In cubic squares or whatever
};

struct Item_Type
{
    Item_Type_ID id;
    const char *name;
    Item_Flags flags;
    State_Of_Matter default_state_of_matter;

    // If (flags & PLACEABLE)
    Placeable placeable;
    
    // If (flags & CONTAINER)
    Container container;
};

struct Container_Modifiers
{
    float volume_multiplier;
};

struct S__Item
{
    Item_Type *type;

    Container_Modifiers container_modifiers;
};

typedef void (Item_Recipe_Proc)(S__Item *product_item, S__Item *ingredient_items);

struct Item_Recipe
{
    struct Ingredient {
        u16 n;
        Item_Type_ID allowed_types[8];
        u8 num_allowed_types;
    };
    
    Item_Type product_type;
    u16 num_products;
    
    Item ingredients[8];
    u16 num_ingredients;

    Item_Recipe_Proc *proc;
};

void pickaxe_recipe_proc(S__Item *product_item, S__Item *ingredient_items, Item_Recipe *recipe)
{
    Ingredient *blade_ingredient = &recipe->ingredients[0];

    for(int i = 0; i < blade_ingredient->n; i++)
    {
        S__Item *item = ingredient_items++;
        
        // Should hit the same case here for every blade_ingredient.
        switch(item->type.id)
        {
            case ITEM_WOODEN_PLANKS: product_item->tool_modifiers.durability = 10; break;
            case ITEM_COBBLESTONE:   product_item->tool_modifiers.durability = 25; break;
            case ITEM_GOLD_INGOT:    product_item->tool_modifiers.durability = 25 + 25 * item->gold_modifiers.carat; break;

            default: Assert(false); break;
        }
    }
};

Item_Recipe item_recipes[] = {
    { ITEM_PICKAXE, 1, {
            { 3, { ITEM_WOODEN_PLANKS, ITEM_COBBLESTONE, ITEM_GOLD_INGOT }, 3 },
            { 2, { ITEM_WOODEN_STICK }, 1 }
        }, 2,
      &pickaxe_recipe_proc
    }
};


void craft(Item_Recipe *recipe, S__Item *ingredient_items, S__Item *_items)
{
    Item_Type *product_type = &item_types[recipe->product_type];

    for(int i = 0; i < recipe->num_products; i++)
    {
        S__Item item = {0};
        item.type = product_type;

        if(recipe->proc) {
            recipe->proc(&item, ingredient_items);
        }
        
        for(int j = 0; j < recipe->num_ingredients; j++)
        {
            auto &ingredient = recipe->ingredients[j];
            ingredient_items += ingredient.n;
        }

        
        *(_items++) = item;
    }
}
