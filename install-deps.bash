#!/usr/bin/env bash

set -e

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
