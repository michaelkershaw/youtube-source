@echo off
echo ========================================
echo Building YouTube Source Installer
echo ========================================
echo.

REM Locate Inno Setup 6 compiler
set ISCC="%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist %ISCC% set ISCC="%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not exist %ISCC% set ISCC="%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if not exist %ISCC% (
    echo ERROR: Inno Setup 6 not found!
    echo Download it from: https://jrsoftware.org/isdl.php
    echo.
    pause
    exit /b 1
)

REM Verify required files exist
if not exist "..\x64\Release\YouTubeSource64.dll" (
    echo ERROR: YouTubeSource64.dll not found. Run build_64.bat first!
    pause
    exit /b 1
)
if not exist "..\yt-dlp.exe" (
    echo ERROR: yt-dlp.exe not found in project root!
    pause
    exit /b 1
)
if not exist "..\ffmpeg-8.0.1-essentials_build\bin\ffmpeg.exe" (
    echo ERROR: ffmpeg.exe not found!
    pause
    exit /b 1
)

%ISCC% YouTubeSource.iss

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo SUCCESS! Installer created in:
    echo installer\output\
    echo ========================================
) else (
    echo.
    echo ERROR: Installer build failed.
)
echo.
pause
