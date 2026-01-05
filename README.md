
# 3D OpenGL Sphere Game with ImGui Controls

![OpenGL Sphere Demo](https://img.shields.io/badge/OpenGL-3.3+-blue.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey.svg)

A real-time 3D graphics application featuring a rotating sphere with interactive controls using ImGui. Built with modern OpenGL, GLFW, GLEW, and SDL3.

## ✨ Features

- **Real-time 3D Sphere Rendering** with Phong lighting model
- **Interactive Controls** via ImGui interface
- **Adjustable Sphere Properties**: Radius, segments, stacks
- **Dynamic Camera Control**: Distance, height, and angle
- **Lighting Controls**: Adjustable light position
- **Visualization Options**: Wireframe mode, background color
- **Performance Monitoring**: Real-time FPS display
- **Auto-rotation** with adjustable speed

## 📋 Prerequisites

- **Windows 10/11** (64-bit)
- **Visual Studio 2022** or newer with C++ support
- **Git** for cloning repositories
- **PowerShell** (for vcpkg installation)
- **CMake** 3.15 or higher

## 🚀 Quick Start

### 1. Install vcpkg Package Manager

Open **PowerShell as Administrator**:

```powershell
# Clone vcpkg repository
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Integrate with Visual Studio (recommended)
.\vcpkg integrate install

# Add vcpkg to PATH (optional but convenient)
$env:Path = "$env:Path;$pwd"
[Environment]::SetEnvironmentVariable("Path", "$env:Path;$pwd", "User")
```

### 2. Install Required Dependencies

```powershell
# Navigate to vcpkg directory
cd C:\vcpkg

# Install all required packages (x64 Windows)
.\vcpkg install --triplet x64-windows ^
    glfw3 ^
    glew ^
    glm ^
    sdl3 ^
    opengl

# Alternatively, install individually:
.\vcpkg install glfw3:x64-windows
.\vcpkg install glew:x64-windows
.\vcpkg install glm:x64-windows
.\vcpkg install sdl3:x64-windows
```

### 3. Clone and Build the Project

```powershell
# Clone the repository
git clone https://github.com/yourusername/opengl-sphere-game.git
cd opengl-sphere-game

# Create build directory
mkdir build
cd build

# Configure with CMake (point to vcpkg toolchain)
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake

# Build the project
cmake --build . --config Release

# Or use the provided batch script (see below)
```

## 📁 Project Structure

```
opengl-sphere-game/
├── CMakeLists.txt          # Main CMake configuration
├── build.bat               # Windows build script
├── README.md              # This file
├── src/
│   └── main.cpp           # Main application code
└── assets/                # (Optional) Future asset files
```

## 🔧 Build Scripts

### Using CMake (Recommended)

```powershell
# Method 1: Using CMake GUI
1. Open CMake GUI
2. Set source directory to project folder
3. Set build directory to "build"
4. Click Configure, then Generate
5. Open generated solution in Visual Studio

# Method 2: Command line
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### Using build.bat (Quick Start)

Create `build.bat` in your project root:

```batch
@echo off
echo ========================================
echo OpenGL Sphere Game Build Script
echo ========================================
echo.

REM Set vcpkg path (adjust if different)
set VCPKG_ROOT=C:\vcpkg

REM Check if vcpkg exists
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo Error: vcpkg not found at %VCPKG_ROOT%
    echo Please install vcpkg first (see README.md)
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -G "Visual Studio 17 2022" -A x64

if errorlevel 1 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build the project
echo Building project...
cmake --build . --config Release

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build successful!
echo ========================================
echo.
echo Output: build\bin\Release\OpenGLSphereGame.exe
echo.
echo To run: build\bin\Release\OpenGLSphereGame.exe
echo.
pause
```

## 🎮 Controls

### Keyboard Shortcuts
- **ESC** - Exit application
- **M** - Toggle ImGui menu visibility
- **W** - Toggle wireframe mode

### ImGui Interface Controls
- **Camera Settings**: Adjust distance, height, and rotation angle
- **Sphere Settings**: Control rotation speed, radius, and auto-rotation
- **Sphere Detail**: Adjust segment and stack counts for tessellation
- **Lighting**: Modify light position in 3D space
- **Display**: Toggle wireframe, change background color
- **Performance**: View real-time FPS and frame time

## 🛠️ Dependencies

### Required Libraries (via vcpkg)
| Library | Version | Purpose |
|---------|---------|---------|
| **GLFW** | 3.3+ | Window and input management |
| **GLEW** | 2.1+ | OpenGL extension loading |
| **GLM** | 0.9.9+ | Mathematics library for graphics |
| **SDL3** | 3.0.0+ | Multimedia library (optional features) |
| **ImGui** | 1.90.4 | Immediate mode GUI (auto-downloaded) |

### Header Files Required
```cpp
// OpenGL and Windowing
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// GUI
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
```

## 🔍 CMake Configuration Details

The project uses CMake with vcpkg integration:

```cmake
# Key CMake features:
- Auto-downloads ImGui via FetchContent
- Automatically finds vcpkg-installed libraries
- Copies necessary DLLs to output directory
- Supports both Debug and Release configurations
- Windows-specific optimizations
```

## 🚨 Troubleshooting

### Common Issues

1. **"SDL3 not found" Error**
   ```powershell
   # Ensure SDL3 is installed
   .\vcpkg install sdl3 --triplet x64-windows
   
   # Check installation
   .\vcpkg list | findstr sdl3
   ```

2. **Missing DLLs at Runtime**
   - The build script automatically copies required DLLs
   - If missing, manually copy from `C:\vcpkg\installed\x64-windows\bin\`
   - Required DLLs: `SDL3.dll`, `glew32.dll`, `glfw3.dll`

3. **CMake Cannot Find vcpkg**
   ```powershell
   # Set VCPKG_ROOT environment variable
   [System.Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
   
   # Or specify toolchain file manually
   cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
   ```

4. **OpenGL Version Issues**
   - Ensure your GPU supports OpenGL 3.3+
   - Update graphics drivers
   - Check with: `glxinfo | grep "OpenGL version"` (Linux) or GPU-Z (Windows)

### Build Errors

| Error | Solution |
|-------|----------|
| `GL/glew.h not found` | Install GLEW: `vcpkg install glew` |
| `GLFW/glfw3.h not found` | Install GLFW: `vcpkg install glfw3` |
| `glm/glm.hpp not found` | Install GLM: `vcpkg install glm` |
| `imgui.h not found` | Let CMake download via FetchContent |
| Linker errors | Ensure all DLLs are in executable directory |

## 📊 Performance Tips

1. **Reduce Sphere Detail** for better FPS
   - Lower segments/stacks in ImGui controls
   - Default: 32 segments, 16 stacks

2. **Disable Anti-aliasing** if needed
   - Modify in code: `glfwWindowHint(GLFW_SAMPLES, 0)`

3. **Use Release Build** for optimal performance
   ```powershell
   cmake --build . --config Release
   ```

## 🧪 Testing

The application includes built-in diagnostics:
- Real-time FPS counter
- Frame time measurement
- OpenGL version detection
- Library initialization status

## 📝 License

MIT License - see [LICENSE](LICENSE) file for details.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Commit your changes
4. Push to the branch
5. Create a Pull Request

## 📚 Resources

- [OpenGL Documentation](https://www.khronos.org/opengl/)
- [GLFW Documentation](https://www.glfw.org/docs/latest/)
- [ImGui Documentation](https://github.com/ocornut/imgui)
- [vcpkg Documentation](https://vcpkg.io/en/)
- [CMake Documentation](https://cmake.org/documentation/)

## 🐛 Bug Reports

Please report issues via GitHub Issues with:
- Operating System and version
- Graphics card and driver version
- Complete error messages
- Steps to reproduce

## 🙏 Acknowledgments

- [ImGui](https://github.com/ocornut/imgui) for the amazing GUI library
- [GLFW](https://www.glfw.org/) for window management
- [GLEW](http://glew.sourceforge.net/) for OpenGL extension loading
- [GLM](https://github.com/g-truc/glm) for mathematics
- [vcpkg](https://vcpkg.io/) for package management

---

**Happy Coding!** 🚀
```

## Additional Files to Include:

### **build.bat** (as mentioned in README)

```batch
@echo off
echo ========================================
echo OpenGL Sphere Game Build Script
echo ========================================
echo.

REM Set vcpkg path (adjust if different)
set VCPKG_ROOT=C:\vcpkg

REM Check if vcpkg exists
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo Error: vcpkg not found at %VCPKG_ROOT%
    echo Please install vcpkg first (see README.md)
    pause
    exit /b 1
)

echo Checking dependencies...
call "%VCPKG_ROOT%\vcpkg.exe" list | findstr "glfw3 glew glm sdl3"

echo.
echo Creating build directory...
if not exist "build" mkdir build
cd build

echo.
echo Configuring with CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" -G "Visual Studio 17 2022" -A x64

if errorlevel 1 (
    echo CMake configuration failed!
    echo.
    echo Troubleshooting tips:
    echo 1. Make sure vcpkg is installed correctly
    echo 2. Run: %VCPKG_ROOT%\vcpkg integrate install
    echo 3. Install missing packages
    pause
    exit /b 1
)

echo.
echo Building project...
cmake --build . --config Release

if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build successful! 🎉
echo ========================================
echo.
echo Executable: build\bin\Release\OpenGLSphereGame.exe
echo.
echo To run the application:
echo 1. Navigate to build\bin\Release\
echo 2. Double-click OpenGLSphereGame.exe
echo.
echo Controls:
echo - ESC: Exit
echo - M: Toggle menu
echo - W: Toggle wireframe
echo.
pause
```

### **.gitignore**

```
# Build directories
build/
out/
bin/
obj/
*.exe
*.dll

# IDE files
.vscode/
.vs/
*.sln
*.vcxproj*
*.user

# OS files
.DS_Store
Thumbs.db

# Dependencies
deps/
libs/

# CMake
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
Makefile

# vcpkg
vcpkg_installed/
```
