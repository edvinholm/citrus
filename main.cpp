
#include <string.h>
#include <math.h>
#include <utility>

#include "types.h"
#include "defer.cpp"

#include "memory_macros.h"

// --

#if DEBUG
#include <stdio.h>
#include "debug.h"
#endif

// --

#include "memory_pre.h"

#include "array.h"

#include "math.h"
#include "math.cpp"

#include "memory.h"
#include "memory.cpp"

#include "array.cpp"

#include "string.h"
#include "string.cpp"

// --

#include "platform.h"

#if OS_WINDOWS
#include <Windows.h>
#include <shellscalingapi.h>
#include "platform_win32.h"
#include "platform_win32.cpp"
#endif

// --

#include "thread.cpp"

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
