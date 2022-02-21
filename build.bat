
@echo off


REM --------
REM local_env.bat should call vcvarsall.bat, and cd into the build directory.
call local_env.bat
REM --------

REM Tell tools that a build is in progress.
echo 1 > tmp/.build_in_progress


echo ----------------------------------------------------------------
DEL vc140.pdb
DEL citrus.pdb
DEL citrus_release.pdb
echo ----------------------------------------------------------------


REM Comment out 'goto' to build client.
:client
REM goto server

Taskkill /IM citrus.exe /F 
Taskkill /IM citrus_release.exe /F

echo ############# COMPILING CLIENT #############

set CLIENT_COMPILER_ARGUMENTS_BASE= -D OS_WINDOWS=1 /EHsc main.cpp /std:c++17
set CLIENT_LINKER_ARGUMENTS_BASE= Shcore.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib opengl32.lib Shell32.lib Comdlg32.lib -incremental:no /opt:ref /opt:icf /nologo

set CLIENT_COMPILER_ARGUMENTS_RELEASE= %CLIENT_COMPILER_ARGUMENTS_BASE%

set CLIENT_COMPILER_EXTRA_ARGUMENTS_DEVELOPER= -D DEVELOPER=1

REM RELEASE BUILD (NOTE: This outputs to another .exe file name than test builds do)
REM cl %CLIENT_COMPILER_ARGUMENTS_RELEASE% /Fecitrus_release.exe /link %CLIENT_LINKER_ARGUMENTS_BASE%

REM DEVELOPER BUILDS ------------
REM Release+Dev:
REM cl %CLIENT_COMPILER_ARGUMENTS_RELEASE% %CLIENT_COMPILER_EXTRA_ARGUMENTS_DEVELOPER% /Fecitrus.exe  /link %CLIENT_LINKER_ARGUMENTS_BASE%

REM Debug:
cl %CLIENT_COMPILER_ARGUMENTS_BASE%  %CLIENT_COMPILER_EXTRA_ARGUMENTS_DEVELOPER% -D DEBUG=1 /Fecitrus.exe -Od /Zo /Z7  /link %CLIENT_LINKER_ARGUMENTS_BASE%



REM Unicode Database Parser
REM cl -D OS_WINDOWS=1 -D DEBUG=1 -D UNICODE_DB_PARSER=1  /EHsc main.cpp /Feucd_parser.exe /Z7 /link winhttp.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib opengl32.lib Comdlg32.lib -incremental:no /opt:ref /opt:icf /nologo


IF %ERRORLEVEL% NEQ 0 goto end
echo Done.

REM Comment out 'goto' to build server.
:server
goto shaders

Taskkill /IM citrus_server.exe /F

echo ----------------------------------------------------------------

echo ############# COMPILING SERVER #############
cl -D OS_WINDOWS=1 -D DEBUG=1 -D SERVER=1  /EHsc main.cpp /Fecitrus_server.exe /Z7 /std:c++17 /link Shcore.lib user32.lib Shell32.lib ws2_32.lib opengl32.lib gdi32.lib -incremental:no /opt:ref /opt:icf /nologo

IF %ERRORLEVEL% NEQ 0 goto end
echo Done.

echo ----------------------------------------------------------------


:shaders
echo Copying shader code...

copy vertex_shader.glsl    "res\vertex_shader.glsl"
copy fragment_shader.glsl  "res\fragment_shader.glsl"

echo Done.

echo ----------------------------------------------------------------

:end


REM Tell tools that a build is no longer in progress.
DEL tmp\.build_in_progress
