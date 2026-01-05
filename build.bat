@echo off
REM Check for vcpkg in common locations
if exist "C:\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    echo Found vcpkg at C:\vcpkg
    set VCPKG_PATH=C:\vcpkg
) else if exist "%USERPROFILE%\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    echo Found vcpkg at %USERPROFILE%\vcpkg
    set VCPKG_PATH=%USERPROFILE%\vcpkg
) else if exist "C:\src\vcpkg\scripts\buildsystems\vcpkg.cmake" (
    echo Found vcpkg at C:\src\vcpkg
    set VCPKG_PATH=C:\src\vcpkg
) else (
    echo Vcpkg not found in common locations!
    echo Please specify the path manually.
    pause
    exit /b 1
)

echo Using vcpkg path: %VCPKG_PATH%
mkdir build 2>nul
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake"
cmake --build .
pause