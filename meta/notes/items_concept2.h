

struct Product
{
	Item_Type_ID type;
	Item_Form_ID form;
	float amount;
};

struct Ingredient
{
	Item_Type_ID type;
	Item_Form_ID form;
	float min_amount;
	float max_amount;
};


struct Item_Type
{
	Item_Type_ID super;

	Item_Type_ID id;
	String name;

	struct Per_Item_Property
	{
		Item_Property_ID property;
		bool default_value_defined;
		float default_value;
	};

	Per_Item_Property per_item_properties[];

	struct Form_Link
	{
		Item_Form_ID previous_form;
		Ingredient additional_ingredients[];
		Item_Type_ID transfer_item;
	};

	struct Form {
		Item_Form_ID form;
		Form_Link links[];
	};

	Form forms[];

	// Naming can be done in a client-side proc. No big deal. See example below.
}


Item_Type *add_item_type(Item_Type *super, String name)
{

}

Item_Type::Per_Item_Property add_per_item_property(Item_Type *type, Item_Property_ID id,
	                                               bool default_value_defined, float default_value = 0)
{

}

// TODO This should be baked before release.
void init_item_types() {
	auto *crops = add_item_type(NULL, "Crops");
	{
		add_prop(crops, ITEM_PROP_PROTEIN,      false); // TODO Should assert that the property does not already exist in this type or any of its super-types.
		add_prop(crops, ITEM_PROP_CARBOHYDRATE, false);
		add_prop(crops, ITEM_PROP_FAT,          false);
		add_prop(crops, ITEM_PROP_WATER,        false);
		add_prop(crops, ITEM_GROWPROG, true, 0);	

		auto *seed      = add_form(crops, ITEM_FORM_SEED);
		auto *growing   = add_form(crops, ITEM_FORM_GROWING);
		auto *harvested = add_form(crops, ITEM_FORM_HARVESTED); // TODO: We should have a way of saying "harvesting crop[growing] will give you <growprog> crop[harvested]"... Or do we do that in code?

		auto *seed_to_growing = add_form_link(growing, ITEM_FORM_SEED, _EARTH_); 
		

		auto *grains = add_item_type(crops, "Grains");
		{
			auto *wheat = add_item_type(grains, "Wheat");
			{
				set_prop_default(wheat, ITEM_PROP_PROTEIN,      10.0); // TODO Should assert that a super-type has the property.
				set_prop_default(wheat, ITEM_PROP_CARBOHYDRATE, 76.0);
				set_prop_default(wheat, ITEM_PROP_FAT,          2.0);
				set_prop_default(wheat, ITEM_PROP_WATER,        13.2);
				Assert(item_type_is_complete(wheat));
			}
			auto *rye = add_item_type(grains, "Rye");
			{
				set_prop_default(rye, ITEM_PROP_PROTEIN,      8.2);
				set_prop_default(rye, ITEM_PROP_CARBOHYDRATE, 60.2);
				set_prop_default(rye, ITEM_PROP_FAT,          2.0);
				set_prop_default(rye, ITEM_PROP_WATER,        14.0);
				Assert(item_type_is_complete(rye));
			}
			auto *oats = add_item_type(grains, "Oats");
			{
				set_prop_default(oats, ITEM_PROP_PROTEIN,      13.2);
				set_prop_default(oats, ITEM_PROP_CARBOHYDRATE, 52.0);
				set_prop_default(oats, ITEM_PROP_FAT,          6.7);
				set_prop_default(oats, ITEM_PROP_WATER,        10.7);
				Assert(item_type_is_complete(oats));
			}
			auto *barley = add_item_type(grains, "Barley");
			{
				set_prop_default(barley, ITEM_PROP_PROTEIN,      9.2);
				set_prop_default(barley, ITEM_PROP_CARBOHYDRATE, 64.7);
				set_prop_default(barley, ITEM_PROP_FAT,          3.0);
				set_prop_default(barley, ITEM_PROP_WATER,        14.0);
				Assert(item_type_is_complete(barley));
			}
		}
	}
	
}	


// @Incomplete: Check that there is no undefined (incomplete) stuff in the type, so that we know if it can be instanted.
bool item_type_is_complete(Item_Type *type)
{

}


/* Naming example

String name_for_item(Item item)
{
	Item_Type *type = find_item_type(item.type);
	String name = type->name;

	if(item.type == bread_type_id)
	{
		float bake_progress = get_property(ITEM_PROP_BAKE_PROGRESS, item);
		if(bake_progress < 1.0) {
			name = concat(name, " Dough");
		} else if(bake_progress > 1.2)
			name = concat ("Burnt", name);
		}
	}

	return name;
}
*/

struct Recipe
{
	Product products[];
	Ingredient ingredients[];
};

// NOTE: This ofc could be baked for each tool.
Recipe[] available_recipes(Item tool)
{
	for(type: item_types)
	{
		for(form: type.forms)
		{
			for(link: type.form_links)
			{
				if(link.transfer_item != tool) continue;

				Recipe recipe = {0};
				for(ingredient: link.ingredients) {
					array_add(recipe.ingredients, ingredient);
				}
				if(link.previous_form != NO_ITEM_FORM) {
					Ingredient self_ingredient = {0};
					self_ingredient.type = type.id;
					self_ingredient.form = link.previous_form;
					self_ingredient.min_amount = 1;
					self_ingredient.max_amount = 1;
					array_add(recipe.ingredients, self_ingredient);
				}

				// NOTE: Here we could have link.biproducts as well... But then we don't know where to define
				// a multi-product link. In this type or the biproduct's type?

				// NOTE: amounts could be scaled up if ingredients/products must be of "integer amounts"...

				Product product = {0};
				product.type   = type;
				product.form   = form;
				product.amount = 1;
				array_add(recipe.products, product);

				array_add(recipes, recipe);	
			}
		}
	}
}