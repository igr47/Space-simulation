cmake_minimum_required(VERSION 3.16)
project(Space-simulation VERSION 1.0.0 LANGUAGES CXX C) 

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Set GLAD paths
set(GLAD_BASE_DIR ${CMAKE_SOURCE_DIR}/external/glad_generated/glad)
set(GLAD_INCLUDE_DIR ${GLAD_BASE_DIR}/include)
set(GLAD_SOURCE_FILE ${GLAD_BASE_DIR}/src/glad.c)

# Verify files exist
if(NOT EXISTS ${GLAD_SOURCE_FILE})
    message(FATAL_ERROR "GLAD source file not found at: ${GLAD_SOURCE_FILE}")
endif()

if(NOT EXISTS ${GLAD_INCLUDE_DIR}/glad/glad.h)
    message(FATAL_ERROR "GLAD header not found at: ${GLAD_INCLUDE_DIR}/glad/glad.h")
endif()

# Create a static library for GLAD
add_library(glad STATIC ${GLAD_SOURCE_FILE})
target_include_directories(glad PUBLIC ${GLAD_INCLUDE_DIR})

# Add stb_image support
set(STB_IMAGE_HEADER ${CMAKE_SOURCE_DIR}/stb_image.h)
if(NOT EXISTS ${STB_IMAGE_HEADER})
    message(FATAL_ERROR "stb_image.h not found at: ${STB_IMAGE_HEADER}")
endif()

# Create an interface library for stb
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_SOURCE_DIR})

# Define STB_IMAGE_IMPLEMENTATION in the main executable
target_compile_definitions(stb_image INTERFACE STB_IMAGE_IMPLEMENTATION)

# Create executable
add_executable(Space-simulation main.cpp)

# Link libraries 
target_link_libraries(Space-simulation
    glad               
    OpenGL::GL
    glfw
    glm::glm
    stb_image
    dl
)

# Also include the GLAD headers
target_include_directories(Space-simulation PRIVATE ${GLAD_INCLUDE_DIR})

# Add compiler flags
target_compile_options(Space-simulation PRIVATE -fPIC)

# Copy texture files to build directory
file(GLOB TEXTURE_FILES 
    ${CMAKE_SOURCE_DIR}/*.jpg
    ${CMAKE_SOURCE_DIR}/*.png
)

foreach(TEXTURE_FILE ${TEXTURE_FILES})
    file(COPY ${TEXTURE_FILE} DESTINATION ${CMAKE_BINARY_DIR}/bin)
endforeach()

# For debugging
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(Space-simulation PRIVATE -g -O0)
endif()

message(STATUS "==========================================")
message(STATUS "GLAD configuration:")
message(STATUS "  Include directory: ${GLAD_INCLUDE_DIR}")
message(STATUS "  Source file: ${GLAD_SOURCE_FILE}")
message(STATUS "==========================================")
message(STATUS "STB configuration:")
message(STATUS "  Header: ${STB_IMAGE_HEADER}")
message(STATUS "==========================================")
