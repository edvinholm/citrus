
#define GFX_GL 1


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

#include "string.h"
#include "string.cpp"

// --

#include "platform.h"

#if OS_WINDOWS
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

#include "memory.cpp"

#include "array.cpp"


#include "string_builder.h"
#include "string_builder.cpp"

#include "file.cpp"
#include "file_read.cpp"
#include "file_write.cpp"

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
    // Check endianness
    {
        u16 x = 1;
        machine_is_big_endian = (*(u8 *)&x == 0);
        Debug_Print("machine_is_big_endian: %d\n", machine_is_big_endian);
    }
    
#if SERVER
    return server_entry_point(num_arguments, arguments);
#else
    return client_entry_point(num_arguments, arguments);    
#endif
}
