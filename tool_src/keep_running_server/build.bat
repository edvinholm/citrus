
@echo off

REM --------
call "../../local_env.bat"
REM --------

cd "tool_src/keep_running_server"

Taskkill /IM keep_running_server.exe /F 

echo ############# COMPILING KRS #############

cl /EHsc main.cpp /Fe../../keep_running_server.exe -D OS_WINDOWS=1 -D DEBUG=1 /Z7 /link -incremental:no /opt:ref /opt:icf /nologo

IF %ERRORLEVEL% NEQ 0 goto end
echo Done.

:end
