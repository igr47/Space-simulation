cmake_minimum_required(VERSION 3.10)
project(AdvancedSolarSystemSimulation)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)

# Find GLU
find_library(GLULIB NAMES GLU)

# Add all source files
add_executable(solar_system
    src/main.cpp
    src/solar_system.cpp
    src/celestialBody.cpp
    src/shader.cpp
)

target_link_libraries(solar_system OpenGL::GL glfw GLEW::GLEW ${GLULIB})
target_include_directories(solar_system PRIVATE include)

# Compiler flags
target_compile_features(solar_system PRIVATE cxx_std_17)
