@echo off
setlocal enabledelayedexpansion

echo Compilazione delle librerie necessarie...

:: Crea le directory necessarie
if not exist "DyMain\lib" mkdir "DyMain\lib"

:: Compila Capstone
echo Compilazione di Capstone...
if not exist "capstone" (
    git clone https://github.com/capstone-engine/capstone.git
    cd capstone
    git checkout 4.0.2
) else (
    cd capstone
    git fetch
    git checkout 4.0.2
)

if not exist "build" mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
copy Release\capstone.lib ..\..\DyMain\lib\
cd ..\..

:: Compila Detours
echo Compilazione di Detours...
if not exist "Detours" (
    git clone https://github.com/microsoft/Detours.git
    cd Detours
) else (
    cd Detours
)

if not exist "build" mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config Release
copy Release\detours.lib ..\..\DyMain\lib\
cd ..\..

echo Compilazione completata!
echo Le librerie sono state copiate in DyMain\lib 