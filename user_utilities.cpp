
#include "calculator.cpp"

struct User_Utilities
{
    Static_Array<Calculator, 4> calculators; // This is just to show an example on how to have data not tied directly to the UI. This could be something more complex, like market connections. You could have multiple views showing information from the same market, but you can not have more than x market connections.
};


// NOTE: Returns id of created calculator, or -1 if max number of calculators are in use.
int create_calculator(User_Utilities *util)
{
    if(util->calculators.n == ARRLEN(util->calculators.e)) return -1;

    int id = 1;
    for(int i = 0; i < util->calculators.n; i++) {
        auto *it = &util->calculators[i];
        if(it->id == id) id++;
    }
    
    Calculator calc = {0};
    calc.id = id;
    calc.num_users = 1;
    array_add(util->calculators, calc);
    
    return id;
}

Calculator *get_calculator(int id, User_Utilities *util, int *_ix = NULL)
{
    for(int i = 0; i < util->calculators.n; i++) {
        auto *it = &util->calculators[i];
        if(it->id == id) {
            if(_ix) *_ix = i;
            return it;
        }
    }

    return NULL;
}

void modify_calculator_users(int id, bool increase, User_Utilities *util)
{
    int ix;
    auto *calc = get_calculator(id, util, &ix);
    Assert(calc);
    
    calc->num_users += (increase) ? 1 : -1;
    Assert(calc->num_users >= 0);
    if(calc->num_users > 0) return;

    array_unordered_remove(util->calculators, ix);
}
