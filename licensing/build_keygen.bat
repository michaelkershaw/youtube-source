@echo off
echo Building License Key Generator...
echo.

:: Try to find Visual Studio C++ compiler
set "VS2026=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat"
set "VS2022=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
set "VS2019=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"

if exist "%VS2026%" (
    call "%VS2026%" x64
) else if exist "%VS2022%" (
    call "%VS2022%" x64
) else if exist "%VS2019%" (
    call "%VS2019%" x64
) else (
    echo ERROR: Visual Studio not found. Please install Visual Studio with C++ tools.
    pause
    exit /b 1
)

echo Compiling generate_key.cpp...
cl /EHsc /std:c++17 generate_key.cpp /Fe:generate_key.exe

if exist generate_key.exe (
    echo.
    echo SUCCESS: generate_key.exe created
    echo.
    echo Usage:
    echo   generate_key.exe
    echo.
    echo This will:
    echo   - Generate a license key
    echo   - Ask for customer name/email
    echo   - Save to registrations.log and keys.csv
    echo.
    del *.obj >nul 2>&1
) else (
    echo ERROR: Compilation failed
)

pause
