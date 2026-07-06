@echo off
echo ========================================
echo YouTube Plugin Installer
echo ========================================
echo.
echo This will copy the newly compiled plugin to VirtualDJ.
echo.
echo IMPORTANT: Make sure VirtualDJ is CLOSED before continuing!
echo.
pause

echo.
echo Copying YouTubeSource64.dll...
copy /Y "x64\Release\YouTubeSource64.dll" "%LOCALAPPDATA%\VirtualDJ\Plugins64\OnlineSources\YouTubeSource64.dll"

echo Copying UI files...
xcopy /Y /E /I "ui" "%LOCALAPPDATA%\VirtualDJ\Plugins64\OnlineSources\youtube-source\ui"

echo Copying tools (yt-dlp, ffmpeg) to youtube-source folder...
if not exist "%LOCALAPPDATA%\VirtualDJ\Plugins64\OnlineSources\youtube-source" mkdir "%LOCALAPPDATA%\VirtualDJ\Plugins64\OnlineSources\youtube-source"
copy /Y "yt-dlp.exe" "%LOCALAPPDATA%\VirtualDJ\Plugins64\OnlineSources\youtube-source\yt-dlp.exe"
copy /Y "ffmpeg-8.0.1-essentials_build\bin\ffmpeg.exe" "%LOCALAPPDATA%\VirtualDJ\Plugins64\OnlineSources\youtube-source\ffmpeg.exe"

if %errorlevel% equ 0 (
    echo.
    echo ========================================
    echo SUCCESS! Plugin installed.
    echo ========================================
    echo.
    echo You can now start VirtualDJ.
    echo The plugin will use the new API endpoints.
    echo.
) else (
    echo.
    echo ========================================
    echo ERROR! Failed to copy the file.
    echo ========================================
    echo.
    echo Make sure VirtualDJ is completely closed!
    echo.
)

pause
