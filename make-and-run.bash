#!/usr/bin/env bash

set -e


# Default flags for gameflix
gameflix_flags=""

# Check if "--release", "--debug", "--no-run", or "--valgrind" flag is passed in
no_run_flag=false
valgrind_flag=false
gameflix_build_type="Release" # Default to release mode

# Check all command-line arguments for flags
for arg in "$@"; do
    if [[ "$arg" == "--no-run" ]]; then
        no_run_flag=true
    elif [[ "$arg" == "--valgrind" ]]; then
        valgrind_flag=true
        gameflix_build_type="Debug"
    elif [[ "$arg" == "--release" ]]; then
        gameflix_build_type="Release"
    elif [[ "$arg" == "--debug" ]]; then
        gameflix_build_type="Debug"
    elif [[ "$arg" == "--help" ]]; then
        echo "Usage: $0 [flags for gameflix]"
        echo ""
        echo "Flags:"
        echo "--help: Display this help message"
        echo "--release: Build in release mode"
        echo "--debug: Build in debug mode"
        echo "--no-run: Only build the app, do not run it"
        echo "--valgrind: Run the app with Valgrind"
        exit 0
    elif [[ "$arg" == "--" ]]; then
        # add the rest of the flags to gameflix_flags
        gameflix_flags="${@:((i+1))}"
        break
    fi
done

# Make the `./build` directory if it doesn't exist

mkdir -p build

# Change to the build directory

cd build

# Run CMake to generate a build.ninja file with the specified build type
cmake -GNinja -DCMAKE_BUILD_TYPE="$gameflix_build_type" ..

# Run ninja to build the app
ninja

# Check if "--no-run" flag is present
if [[ "$no_run_flag" == true ]]; then
    echo "Build completed. Run the app manually with './bin/gameflix ${args[*]}'"
elif [[ "$valgrind_flag" == true ]]; then
    # Change back to the project root directory
    cd ..

    # Execute the `valgrind` command with the `./bin/gameflix` binary and user-provided flags
    #valgrind --leak-check=full ./bin/gameflix "${args[@]}" ${gameflix_flags}
    valgrind ./bin/gameflix "${args[@]}" ${gameflix_flags}
else
    # Change back to the project root directory
    cd ..

    # Execute the `./bin/gameflix` binary with user-provided flags
    ./bin/gameflix "${args[@]}" ${gameflix_flags}
fi
