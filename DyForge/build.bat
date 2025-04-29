@echo off
setlocal enabledelayedexpansion

:: Configurazione
set "BUILD_DIR=build"
set "BIN_DIR=bin"
set "SRC_DIR=src"
set "INCLUDE_DIR=include"
set "LIB_DIR=lib"

:: Crea le directory se non esistono
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"

:: Compila i file sorgente
echo Compilando i file sorgente...

:: Compila InjectionManager.cpp
cl.exe /nologo /EHsc /W4 /Zi /Od /I"%INCLUDE_DIR%" /I"..\DyHexInject" /c "%SRC_DIR%\InjectionManager.cpp" /Fo"%BUILD_DIR%\InjectionManager.obj"
if errorlevel 1 (
    echo Errore durante la compilazione di InjectionManager.cpp
    exit /b 1
)

:: Compila UIManager.cpp
cl.exe /nologo /EHsc /W4 /Zi /Od /I"%INCLUDE_DIR%" /c "%SRC_DIR%\ui\UIManager.cpp" /Fo"%BUILD_DIR%\UIManager.obj"
if errorlevel 1 (
    echo Errore durante la compilazione di UIManager.cpp
    exit /b 1
)

:: Compila CommunicationManager.cpp
cl.exe /nologo /EHsc /W4 /Zi /Od /I"%INCLUDE_DIR%" /c "%SRC_DIR%\communication\CommunicationManager.cpp" /Fo"%BUILD_DIR%\CommunicationManager.obj"
if errorlevel 1 (
    echo Errore durante la compilazione di CommunicationManager.cpp
    exit /b 1
)

:: Linka l'eseguibile
echo Linkando l'eseguibile...
link.exe /nologo /DEBUG /SUBSYSTEM:WINDOWS /OUT:"%BIN_DIR%\DyForge.exe" ^
    "%BUILD_DIR%\InjectionManager.obj" ^
    "%BUILD_DIR%\UIManager.obj" ^
    "%BUILD_DIR%\CommunicationManager.obj" ^
    "..\DyHexInject\bin\DyHexInject.lib" ^
    user32.lib gdi32.lib comctl32.lib shlwapi.lib comdlg32.lib psapi.lib
if errorlevel 1 (
    echo Errore durante il linking
    exit /b 1
)

:: Copia le DLL necessarie
echo Copiando le DLL...
copy "..\DyHexInject\bin\DyHexInject.dll" "%BIN_DIR%"
copy "..\DyMain\bin\DyMain.dll" "%BIN_DIR%"

echo Build completata con successo!
endlocal 