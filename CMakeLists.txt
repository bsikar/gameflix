# Specify the minimum required version of CMake
cmake_minimum_required(VERSION 3.0)

# Set the project name to "Gamefilx"
project(Gameflix)

# Set the C++ standard to 17
set(CMAKE_CXX_STANDARD 17)

# Add compiler flags to the C++ compiler
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -Wextra -Weffc++ -Wunused -pedantic -fsanitize=address,undefined")

# Set variables for the source directory, build directory, binary directory, and assets directory
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(BUILD_DIR ${SRC_DIR}/build)
set(BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)

# Use file() to find all source files and header files in the source directory
file(GLOB SRC_FILES ${SRC_DIR}/*.cpp)
file(GLOB_RECURSE HEADER_FILES ${SRC_DIR}/*.h)

# Create an executable target called "program" using the source and header files
add_executable(program ${SRC_FILES} ${HEADER_FILES})

# Link the necessary libraries to the target
target_link_libraries(program avformat avcodec avutil swresample)

# Set the target properties for the "program" executable, including the output directory
set_target_properties(program PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
        )

# Include the source directory as a private include directory
target_include_directories(program PRIVATE ${SRC_DIR})

# Set the target properties for the "program" executable again, just in case
set_target_properties(program PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
        )

# Create a custom target called "copy_assets" that copies the assets directory to the binary directory
add_custom_target(copy_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ASSETS_DIR} ${BIN_DIR}
        )

# Create a custom command that copies the assets directory to the binary directory after building the "program" target
add_custom_command(TARGET program POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ASSETS_DIR} ${BIN_DIR}
        )

