

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
    

    
    String titles[]    = { STRING("Name"), STRING("Age"), STRING("Location"), STRING("Actions") };
    float  fractions[] = {     .4,    .1,         .2,        .3 };
    static_assert(ARRLEN(titles) == ARRLEN(fractions));
    begin_list(P(ctx), titles, fractions, ARRLEN(titles));
    {
        for(int i = 0; i < ARRLEN(people); i++) {
            auto *it = &people[i];
            list_cell(PC(ctx, i), it->name);
            list_cell(PC(ctx, i), EMPTY_STRING);
            list_cell(PC(ctx, i), EMPTY_STRING);
            list_cell(PC(ctx, i), EMPTY_STRING);
        }
    }
    end_list(P(ctx));

    
    _TOP_(24);
    static float x = .3;
    x = slider(P(ctx), x);

}
