@echo off
setlocal enabledelayedexpansion
echo Building YouTube Source Plugin (64-bit only)...

REM Try to find vcvarsall.bat using vswhere
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" (
    set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
)

if exist "!VSWHERE!" (
    for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -find **\vcvarsall.bat`) do (
        set "VCVARSALL=%%i"
    )
)

if not defined VCVARSALL (
    echo Visual Studio vcvarsall.bat not found.
    echo Please ensure Visual Studio is installed correctly.
    pause
    exit /b 1
)

echo Using environment from: "!VCVARSALL!"

REM Build 64-bit version
echo Building 64-bit version...
cmd /c "call "!VCVARSALL!" x64 && msbuild YouTubeSource.vcxproj /p:Configuration=Release /p:Platform=x64 /verbosity:minimal"
if !ERRORLEVEL! NEQ 0 (
    echo 64-bit build failed!
    pause
    exit /b 1
)

echo.
echo Build completed successfully
echo.
echo File created:
echo - x64\Release\YouTubeSource64.dll (64-bit)
echo.
echo To install, copy to: %LOCALAPPDATA%\VirtualDJ\Plugins64\OnlineSources\
echo.
pause
