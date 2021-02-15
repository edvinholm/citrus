
enum Price_Period {
    PERIOD_HOUR  = 0,
    PERIOD_DAY   = 1,
    PERIOD_WEEK  = 2,
    PERIOD_MONTH = 3,
    PERIOD_YEAR  = 4,

    PERIOD_NONE_OR_NUM
};
String price_period_names[] = {
    STRING("HOUR"),
    STRING("DAY"),
    STRING("WEEK"),
    STRING("MONTH"),
    STRING("YEAR")
};
static_assert(ARRLEN(price_period_names) == PERIOD_NONE_OR_NUM);

enum Market_View_Target_Type
{
    MARKET_VIEW_TARGET_ARTICLE,
    MARKET_VIEW_TARGET_ORDERS
};

struct Market_View_Target
{
    Market_View_Target_Type type;

    union {
        struct {
            Item_Type_ID article; // This can be ITEM_NONE_OR_NUM.
            Price_Period price_period;
        } article;
    };
};

struct S__Market_View
{
    Market_View_Target target;

    union {
        struct {
            u16 num_prices;
            Money prices[128];
        } article;

        struct {
        } orders;
        
    };
};

namespace Client_Market
{
    struct Market_View: public S__Market_View
    {

    };
};

namespace Server_Market
{
    struct Market_View: public S__Market_View
    {

    };
};
