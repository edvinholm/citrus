
#define Fail_If_True(Condition) \
    if(Condition) { Debug_Print("[FAILURE] Condition met: %s at %s:%d\n", #Condition, __FILE__, __LINE__); return false; }


#define GFX_GL 1

#ifndef DEVELOPER
#define DEVELOPER 0
#endif


#include <string.h>
#include <math.h>
#include <utility>

#include "types.h"
#include "defer.cpp"

#include "memory_macros.h"

// --
const u16 ROOM_SERVER_PORT   = 50777;
const u16 USER_SERVER_PORT   = 50888;
const u16 MARKET_SERVER_PORT = 50999;
// --

#if DEBUG
#include <stdio.h>
#endif

#include "debug.h"
#include "profile.h"

// --

#include "memory_pre.h"

#include "array.h"

#include "math.h"
#include "math.cpp"

#include "memory.h"

#include "string.h"
#include "string.cpp"

// --


struct App_Version
{
    u16 comp[4];
};
bool equal(App_Version &a, App_Version &b)
{
    for(int i = 0; i < ARRLEN(a.comp); i++)
        if(a.comp[i] != b.comp[i]) return false;

    return true;
}

#if UNICODE_DB_PARSER
#elif SERVER
App_Version APP_version = {{
#include "_app_version_server"
}};
#else
App_Version APP_version = {{
#include "_app_version_client"
}};
#endif


// --

#include "platform.h"

#if OS_WINDOWS
#include <xmmintrin.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <shellscalingapi.h>
#include "platform_win32.h"

#include <GL\gl.h>
#include "gpu_gl.h"
#include "gpu.h"
#include "gl.h"
#include "gl.cpp"
#include "shaders.h"
#include "gpu_gl.cpp"

#include "platform_win32.cpp"
#endif

// --

#include "tweaks.h"

// --

#include "memory.cpp"

#include "array.cpp"


#include "string_builder.h"
#include "string_builder.cpp"

#include "file.cpp"
#include "file_read.cpp"
#include "file_write.cpp"

#include "developer.cpp"
#include "tweaks.cpp"

#include "thread.cpp"

#if SERVER
#include "server_includes.h"
#include "server.cpp"
#else
#include "client_includes.h"
#include "client.cpp"
#endif


#if UNICODE_DB_PARSER
#include "unicode_db.cpp"
#endif


int main(int num_arguments, char **arguments)
{
    // Check endianness
    {
        u16 x = 1;
        machine_is_big_endian = (*(u8 *)&x == 0);
        Debug_Print("machine_is_big_endian: %d\n", machine_is_big_endian);
    }

#if UNICODE_DB_PARSER
    return unicode_db_entry_point(num_arguments, arguments);
#elif SERVER
    return server_entry_point(num_arguments, arguments);
#else
    return client_entry_point(num_arguments, arguments);    
#endif
}
