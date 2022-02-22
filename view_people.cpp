

struct Person {
    String name;
};


void people_view(UI_Context ctx)
{
    U(ctx);

    Person people[] = {
        { STRING("Kombucha Baba") },
        { STRING("Marclat Picco") },
        { STRING("Fendur Meyz") },
        { STRING("Paccaq Dem") },
        { STRING("Benk Zimande") },
        { STRING("Fuppu Raloon") }
    };
    

    for(int i = 0; i < ARRLEN(people); i++) {
        _TOP_SLIDE_(24);
        auto *it = &people[i];
        button(PC(ctx, i), it->name);
    }

    
}
