@echo off
setlocal enabledelayedexpansion

:: Compiler settings
set CC=cl.exe
set CFLAGS=/nologo /W3 /O2 /D_CRT_SECURE_NO_WARNINGS /D_WIN32_WINNT=0x0601

:: Create output directory
if not exist bin mkdir bin

:: Compile DyHexInject
echo Compiling DyHexInject...
%CC% %CFLAGS% /c DyHexInject.c /Fo:bin\DyHexInject.obj
if errorlevel 1 goto :error

:: Compile example
echo Compiling example...
%CC% %CFLAGS% /c example.c /Fo:bin\example.obj
if errorlevel 1 goto :error

:: Link DyHexInject
echo Linking DyHexInject...
%CC% %CFLAGS% bin\DyHexInject.obj /link /OUT:bin\DyHexInject.dll /DLL
if errorlevel 1 goto :error

:: Link example
echo Linking example...
%CC% %CFLAGS% bin\example.obj /link /OUT:bin\example.exe
if errorlevel 1 goto :error

echo Build completed successfully!
goto :end

:error
echo Build failed!
exit /b 1

:end
endlocal 