


#define Global_Log(...) \
    printf(__VA_ARGS__)

#define Global_Log_T(Tag, ...) \
    printf("[%s] ", Tag);          \
    Global_Log(__VA_ARGS__)
