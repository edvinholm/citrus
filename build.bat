
@echo off


REM --------
REM local_env.bat should call vcvarsall.bat, and cd into the build directory.
call local_env.bat
REM --------


echo ----------------------------------------------------------------
Taskkill /IM citrus.exe /F
Taskkill /IM citrus_release.exe /F
echo ----------------------------------------------------------------
DEL vc140.pdb
DEL citrus.pdb
DEL citrus_release.pdb
echo ----------------------------------------------------------------

echo ############# COMPILING CLIENT #############

REM RELEASE BUILD (NOTE: This outputs to another .exe file name than test builds do)
REM cl -D OS_WINDOWS=1 -D DEBUG=0 -D  /EHsc main.cpp /Fecitrus_release.exe /Z7 /link Shcore.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib opengl32.lib Shell32.lib Comdlg32.lib -incremental:no /opt:ref /opt:icf /nologo

REM TEST BUILDS ------------
REM "Release":
REM cl -D OS_WINDOWS=1 -D DEBUG=0  /EHsc main.cpp /Fecitrus.exe /Z7 /link Shcore.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib opengl32.lib Shell32.lib Comdlg32.lib -incremental:no /opt:ref /opt:icf /nologo
REM Debug:
cl -D OS_WINDOWS=1 -D DEBUG=1  /EHsc main.cpp /Fecitrus.exe /Z7 /link Shcore.lib ws2_32.lib kernel32.lib user32.lib gdi32.lib opengl32.lib Shell32.lib Comdlg32.lib -incremental:no /opt:ref /opt:icf /nologo


IF %ERRORLEVEL% NEQ 0 EXIT 1
echo Done.

echo ----------------------------------------------------------------

echo ############# COMPILING SERVER #############
cl -D OS_WINDOWS=1 -D DEBUG=1 -D SERVER=1  /EHsc main.cpp /Fecitrus_server.exe /Z7 /link -incremental:no /opt:ref /opt:icf /nologo

IF %ERRORLEVEL% NEQ 0 EXIT 1
echo Done.

REM echo ----------------------------------------------------------------

REM echo Copying shader code...

REM copy vertex_shader.glsl         "res\vertex_shader.glsl"
REM copy fragment_shader.glsl         "res\fragment_shader.glsl"

REM echo Done.

REM echo ----------------------------------------------------------------

IF %ERRORLEVEL% NEQ 0 EXIT 1
