cmake_minimum_required(VERSION 3.15)
project(OpenGLSphereGame)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Download and include ImGui using FetchContent
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4
)

FetchContent_MakeAvailable(imgui)

# Add ImGui backend sources directly
set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Find packages (vcpkg installed)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# Find SDL3 - use the correct vcpkg path
# First check common vcpkg locations
set(VCPKG_PATHS
    "C:/Users/PC/vcpkg/installed/x64-windows"
    "C:/vcpkg/installed/x64-windows"
    "$ENV{VCPKG_ROOT}/installed/x64-windows"
)

foreach(VCPKG_PATH ${VCPKG_PATHS})
    if(EXISTS "${VCPKG_PATH}/include/SDL3/SDL.h")
        message(STATUS "Found SDL3 at: ${VCPKG_PATH}")
        set(SDL3_PATH "${VCPKG_PATH}")
        break()
    endif()
endforeach()

if(SDL3_PATH)
    # Set SDL3 paths manually
    set(SDL3_INCLUDE_DIR "${SDL3_PATH}/include/SDL3")
    set(SDL3_LIBRARY "${SDL3_PATH}/lib/SDL3.lib")
    set(SDL3_MAIN_LIBRARY "${SDL3_PATH}/lib/SDL3main.lib")
    
    message(STATUS "SDL3 include directory: ${SDL3_INCLUDE_DIR}")
    message(STATUS "SDL3 library: ${SDL3_LIBRARY}")
    
    # Check if files exist
    if(NOT EXISTS "${SDL3_INCLUDE_DIR}/SDL.h")
        message(WARNING "SDL3 header not found at: ${SDL3_INCLUDE_DIR}/SDL.h")
    endif()
    
    if(NOT EXISTS "${SDL3_LIBRARY}")
        message(WARNING "SDL3 library not found at: ${SDL3_LIBRARY}")
    endif()
else()
    message(WARNING "SDL3 not found via vcpkg. SDL3 features will be disabled.")
    message(STATUS "To install SDL3: vcpkg install sdl3 --triplet x64-windows")
endif()

# Create executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${IMGUI_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Add SDL3 include if found
if(SDL3_PATH)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_INCLUDE_DIR})
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_SDL3=1)
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME}
    OpenGL::GL
    glfw
    GLEW::GLEW
)

# Add SDL3 libraries if found
if(SDL3_PATH)
    target_link_libraries(${PROJECT_NAME} ${SDL3_LIBRARY})
endif()

# Windows specific settings
if(WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32)
    
    # Add SDL3 main library for Windows
    if(SDL3_PATH AND EXISTS "${SDL3_MAIN_LIBRARY}")
        target_link_libraries(${PROJECT_NAME} ${SDL3_MAIN_LIBRARY})
    endif()
    
    # Copy DLLs to output directory
    if(SDL3_PATH AND EXISTS "${SDL3_PATH}/bin/SDL3.dll")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SDL3_PATH}/bin/SDL3.dll"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
            COMMENT "Copying SDL3.dll to output directory"
        )
    endif()
    
    # Copy GLFW DLL if needed
    get_target_property(GLFW_LIB_TYPE glfw TYPE)
    if(GLFW_LIB_TYPE STREQUAL "SHARED_LIBRARY")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:glfw>"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        )
    endif()
    
    # Copy GLEW DLL
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:GLEW::GLEW>"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    )
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "=========================================")
message(STATUS "Configuration Summary")
message(STATUS "=========================================")
message(STATUS "Project:        ${PROJECT_NAME}")
message(STATUS "Build Type:     ${CMAKE_BUILD_TYPE}")
message(STATUS "OpenGL:         Found")
message(STATUS "GLFW:           Found")
message(STATUS "GLEW:           Found")
message(STATUS "ImGui:          Downloaded via FetchContent")
if(SDL3_PATH)
    message(STATUS "SDL3:           Found at ${SDL3_PATH}")
else()
    message(STATUS "SDL3:           NOT FOUND")
endif()
message(STATUS "Output Dir:     ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "=========================================")