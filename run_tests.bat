@echo off
echo ========================================================
echo   Page Replacement Simulator - Running Automated Tests
echo ========================================================

REM Add w64devkit to PATH if present
if exist "C:\tools\w64devkit\bin" (
    set "PATH=C:\tools\w64devkit\bin;%PATH%"
)

if not exist "build\backend\test_algorithms.exe" (
    echo [INFO] Compiling tests with CMake...
    cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    cmake --build build
)

echo.
echo [1/2] Running C++ Native Unit Tests...
.\build\backend\test_algorithms.exe

echo.
echo [2/2] Running Python End-to-End REST API Invariant Tests...
python tests\test_api_e2e.py

echo.
echo ========================================================
echo   ALL TESTS FINISHED
echo ========================================================
pause
