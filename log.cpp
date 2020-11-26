



#define Log(...) \
    printf(__VA_ARGS__)

#define Log_T(Tag, ...) \
    printf("[%s] ", Tag);          \
    Log(__VA_ARGS__)
