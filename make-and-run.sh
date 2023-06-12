#!/usr/bin/env bash

set -e

# This script is meant to do the following
# ** note that this script is not supported
# but this might work and I (and the other developers)
# might use this to build and run the code + install
# dependencies **

# Default flags for gameflix
gameflix_flags=""

# Check if "--help" is passed in
if [[ "$1" == "--help" ]]; then
    echo "Usage: $0 [flags for gameflix]"
    echo ""
    echo "Flags:"
    echo "--help: Display this help message"
    exit 0
fi

# Check if any flags are passed in
if [[ $# -gt 0 ]]; then
    # Check if "--" is present and get the flags after it
    if [[ "$1" == "--" ]]; then
        gameflix_flags="${@:2}"
    else
        gameflix_flags="$@"
    fi
fi

# Define list of dependencies
dependencies=(
    ffmpeg
    cmake
    ninja
    clang
)

# Check if dependencies are installed and install if necessary
for dependency in "${dependencies[@]}"
do
    if ! command -v "$dependency" &> /dev/null
    then
        echo "$dependency is not installed. Installing..."
        case "$(uname -s)" in
            Darwin)
                brew install "$dependency"
                ;;
            Linux)
                if [[ -n $(command -v apt-get) ]]; then
                    sudo apt-get update -y && sudo apt-get install -y "$dependency"
                elif [[ -n $(command -v pacman) ]]; then
                    sudo pacman -Sy --noconfirm "$dependency"
                else
                    echo "Unsupported package manager or distribution."
                    exit 1
                fi
                ;;
            *)
                echo "Unsupported operating system."
                exit 1
                ;;
        esac
    fi
done

# Make the `./build` directory if it doesn't exist
mkdir -p build

# Change to build directory
cd build

# Run CMake to generate a build.ninja file
cmake -GNinja ..

# Run ninja to build the app
ninja

# Change back to project root directory
cd ..

# Execute the `./bin/gameflix` binary with user-provided flags
# check if the gameflix_flags are empty
if [[ -z "$gameflix_flags" ]]; then
    ./bin/gameflix
else
    ./bin/gameflix ${gameflix_flags[@]}
fi
