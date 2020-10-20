
#define STRING_BUILDER_INITIAL_BUFFER_SIZE 256

//IMPORTANT: :StringBuilderResetResponsability
//           You are responsible for resetting the builder
//           when you are done with it - you should not need
//           to reset it before start appending stuff.

struct String_Builder
{
    u8 *buffer;
    strlength length;
    
    strlength capacity;
};
