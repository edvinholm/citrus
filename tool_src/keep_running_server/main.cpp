
#include <stdio.h>
#include <Windows.h>

#include <utility>
#include "../../defer.cpp"

// Not @ThreadSafe (@Robustness)
bool should_exit = false;
bool process_is_running = false;
PROCESS_INFORMATION process_info = {0};
//--

BOOL WINAPI console_ctrl_handler(DWORD signal) {

    if (signal == CTRL_C_EVENT ||
        signal == CTRL_CLOSE_EVENT)
    {
        TerminateProcess(process_info.hProcess, 99);
        should_exit = true;
        return TRUE;
    }
    
    return FALSE;
}

bool file_exists(char *filename)
{
   WIN32_FIND_DATA FindFileData;
   HANDLE handle = FindFirstFile(filename, &FindFileData) ;
   if(handle != INVALID_HANDLE_VALUE) {
       FindClose(handle);
       return true;
   }
   return false;
}

void wait_for_green_light()
{
    while(file_exists("tmp/.build_in_progress")) {
        Sleep(100);
    }
}

int main(int num_arguments, char **arguments)
{
    printf("I am KRS (Keep Running Server(TM)).\n");

    SetConsoleCtrlHandler(&console_ctrl_handler, TRUE);

    while(true)
    {
        wait_for_green_light();
            
        printf("Launching server...");
        
        STARTUPINFO startup_info = {0};
        startup_info.cb = sizeof(startup_info);

        memset(&process_info, 0, sizeof(process_info));
        /* IMPORTANT: process_info.hProcess,
                              .hThread,
                              needs to be closed. 
        */    
        if(!CreateProcess("citrus_server.exe",
                          NULL,
                          NULL,
                          NULL,
                          FALSE,
                          NORMAL_PRIORITY_CLASS | CREATE_NEW_PROCESS_GROUP,
                          NULL,
                          NULL,
                          &startup_info,
                          &process_info))
        {
            printf("Failed to launch server.\n");
            return 1;
        }
               process_is_running = true;

        defer(CloseHandle(process_info.hProcess););
        defer(CloseHandle(process_info.hThread););
        printf("Server is running.\n");


        WaitForSingleObject(process_info.hProcess, INFINITE);
        process_is_running = false;
        
        printf("Server exited.\n");
        if(should_exit) break;
    }
    
    return 0;
}
