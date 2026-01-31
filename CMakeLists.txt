cmake_minimum_required(VERSION 3.20)
project(ImageContourRenderer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(OpenCV REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# ImGui
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")
file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/*.cpp
)
list(FILTER IMGUI_SOURCES EXCLUDE REGEX ".*demo\\.cpp$")
list(FILTER IMGUI_SOURCES EXCLUDE REGEX ".*test\\.cpp$")

# TinyFileDialogs
set(TINYFD_DIR "${CMAKE_SOURCE_DIR}/third_party/tinyfiledialogs")

# Source files
set(SOURCES
    src/main.cpp
    src/App.cpp
    src/ImageProcessor.cpp
    src/Renderer.cpp
    ${TINYFD_DIR}/tinyfiledialogs.c
    ${IMGUI_SOURCES}
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Include directories
set(INCLUDE_DIRS
    include
    ${TINYFD_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${OpenCV_INCLUDE_DIRS}
)

# Create executable
add_executable(ImageContourRenderer ${SOURCES})

target_include_directories(ImageContourRenderer PRIVATE ${INCLUDE_DIRS})
target_link_libraries(ImageContourRenderer PRIVATE 
    OpenGL::OpenGL
    glfw
    GLEW::GLEW
    ${OpenCV_LIBS}
)

# Copy assets to build directory
add_custom_command(TARGET ImageContourRenderer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/assets
)
