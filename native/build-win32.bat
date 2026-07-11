@echo off
setlocal

set "NATIVE_DIR=%~dp0"
set "BUILD_DIR=%NATIVE_DIR%build-win32"

cmake -S "%NATIVE_DIR%" -B "%BUILD_DIR%" -G "Visual Studio 17 2022" -A Win32
if errorlevel 1 exit /b %errorlevel%

cmake --build "%BUILD_DIR%" --config Release
if errorlevel 1 exit /b %errorlevel%

echo.
echo Built 32-bit Windows executable:
echo %BUILD_DIR%\Release\wbwwb_native.exe
