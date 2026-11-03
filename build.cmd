@echo off
rem Build Screzy.exe on Windows with MinGW-w64.
rem Needs gcc and windres on PATH, e.g. from w64devkit (https://github.com/skeeto/w64devkit)
rem or MSYS2 (pacman -S mingw-w64-x86_64-gcc).
setlocal
cd /d "%~dp0"
if not exist build mkdir build

windres -I res res\screzy.rc -O coff -o build\res.o || goto :fail
gcc -O2 -Wall -mwindows -static -o build\Screzy.exe src\screzy.c build\res.o ^
    -lwinmm -ldwmapi -lcomctl32 -lshlwapi -luser32 -lshell32 -ladvapi32 -lgdi32 || goto :fail

echo Built build\Screzy.exe
exit /b 0

:fail
echo.
echo Build failed. Make sure MinGW-w64 is installed and gcc + windres are on PATH.
exit /b 1
