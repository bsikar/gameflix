# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.0)

# Set the project name to "Gamefilx"
project(Gameflix)

# Set the C++ standard to 17
set(CMAKE_CXX_STANDARD 17)

# Add compiler flags to the C++ compiler
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -Wno-unused -Wno-unused-parameter -Wno-unused-function -Wno-c++98-compat -Wno-exit-time-destructors -Wno-global-constructors -Wno-unused-lambda-capture -Wno-c++17-extensions -Wno-gnu-zero-variadic-macro-arguments -std=c++17 -O3 -g -fsanitize=address,undefined")

# Set variables for the source directory, build directory, binary directory, and assets directory
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(BUILD_DIR ${SRC_DIR}/build)
set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)

# Set the compiler to be clang++ instead of g++
set(CMAKE_CXX_COMPILER "clang++")

# Use file() to find all source files and header files in the source directory
file(GLOB SRC_FILES ${SRC_DIR}/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${SRC_DIR}/*.h)

# Create an executable target called "gameflix" using the source and header files
add_executable(gameflix ${SRC_FILES} ${HEADER_FILES})

# Use pkg-config to locate the necessary FFmpeg libraries and header files
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswresample)

# Link the necessary libraries to the target
target_link_libraries(gameflix ${FFMPEG_LIBRARIES})

# Include the necessary header files
target_include_directories(gameflix PRIVATE ${FFMPEG_INCLUDE_DIRS})

# Set the target properties for the "gameflix" executable, including the output directory
set_target_properties(gameflix PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
        )

# Include the source directory as a private include directory
target_include_directories(gameflix PRIVATE ${SRC_DIR})

# Set the target properties for the "gameflix" executable again, just in case
set_target_properties(gameflix PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
        )
