
#include <utility>

#include "types.h"
#include "defer.cpp"

#if DEBUG
#include <stdio.h>
#include "debug.h"
#endif

#if SERVER
#include "server_includes.h"
#include "server.cpp"
#else
#include "client_includes.h"
#include "client.cpp"
#endif



int main(int num_arguments, char **arguments)
{
#if SERVER
    return server_entry_point(num_arguments, arguments);
#else
    return client_entry_point(num_arguments, arguments);    
#endif
}
