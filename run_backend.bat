@echo off
echo ========================================================
echo   Page Replacement Simulator - Starting C++ Backend
echo ========================================================

REM Add w64devkit to PATH if present
if exist "C:\tools\w64devkit\bin" (
    set "PATH=C:\tools\w64devkit\bin;%PATH%"
)

REM Build if executable not present
if not exist "build\backend\page_replacement_server.exe" (
    echo [INFO] Executable not found. Compiling backend with CMake...
    cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    cmake --build build
)

if exist "build\backend\page_replacement_server.exe" (
    echo [INFO] Launching server on http://localhost:8080...
    echo [INFO] Open your web browser at: http://localhost:8080
    echo [INFO] Press Ctrl+C to terminate the server.
    .\build\backend\page_replacement_server.exe 8080
) else (
    echo [ERROR] Failed to find or build page_replacement_server.exe.
    pause
)
