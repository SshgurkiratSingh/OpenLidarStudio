cmake_minimum_required(VERSION 3.20)
project(OpenLidarStudio VERSION 1.5.5 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler flags
if(MSVC)
    # /W3 for our code, suppress known noisy warnings from YDLIDAR SDK headers
    add_compile_options(
        /W3
        /O2
        /permissive-
        /Zc:preprocessor
        /wd4251   # dll-interface warnings from CYdLidar.h (3rdparty)
        /wd4324   # structure padded due to alignment (ring_buffer)
        /wd4189   # local variable initialised but not referenced
        /WX-      # Do not treat warnings as errors
    )
    # Suppress MSVC secure-function deprecation warnings in 3rdparty code
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS
        _WINSOCK_DEPRECATED_NO_WARNINGS
        NOMINMAX
    )
else()
    add_compile_options(-Wall -Wextra -Wpedantic -O3)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-Wno-interference-size)
    endif()
endif()

# Embedded YDLIDAR SDK Integration
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/ydlidar_sdk/CMakeLists.txt")
    set(BUILD_TEST OFF CACHE BOOL "" FORCE)
    set(BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(3rdparty/ydlidar_sdk)
else()
    message(FATAL_ERROR "YDLIDAR SDK submodule not found in 3rdparty/ydlidar_sdk!")
endif()

# Dependency Management (CPM.cmake)
include(cmake/CPM.cmake)

CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.3.8
    OPTIONS "GLFW_BUILD_DOCS OFF" "GLFW_BUILD_TESTS OFF" "GLFW_BUILD_EXAMPLES OFF"
)

CPMAddPackage(
    NAME glad
    GITHUB_REPOSITORY Dav1dde/glad
    GIT_TAG v0.1.36
    OPTIONS "GLAD_PROFILE core" "GLAD_API gl=3.3"
)

CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 0.9.9.8
)

CPMAddPackage(
    NAME imgui
    GITHUB_REPOSITORY ocornut/imgui
    GIT_TAG v1.90.4-docking
    DOWNLOAD_ONLY YES
)

CPMAddPackage(
    NAME implot
    GITHUB_REPOSITORY epezent/implot
    GIT_TAG v0.16
    DOWNLOAD_ONLY YES
)

# ImGui & ImPlot Target Creation
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${implot_SOURCE_DIR}
)
target_link_libraries(imgui_lib PUBLIC glfw glad)

# Application Target
file(GLOB_RECURSE SRC_FILES src/*.cpp)
# We might not have any src files yet, but CMake doesn't like add_executable without sources.
# Let's add an empty main.cpp first before configuring CMake.
if(SRC_FILES)
    add_executable(OpenLidarStudio ${SRC_FILES})
else()
    add_executable(OpenLidarStudio src/main.cpp)
endif()

target_include_directories(OpenLidarStudio PRIVATE
    include
)
# Mark 3rdparty SDK headers as SYSTEM so MSVC/GCC suppresses their warnings
target_include_directories(OpenLidarStudio SYSTEM PRIVATE
    3rdparty/ydlidar_sdk/include
    3rdparty/ydlidar_sdk/src
)

target_link_libraries(OpenLidarStudio PRIVATE
    ydlidar_driver
    imgui_lib
    glm::glm
    glad
    glfw
    ${CMAKE_DL_LIBS}
)

if(WIN32)
    target_link_libraries(OpenLidarStudio PRIVATE setupapi ws2_32)
endif()
