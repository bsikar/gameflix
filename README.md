# Gameflix
A tool that generates short videos by blending game play footage with scenes from popular movies and shows, perfect for sharing on social media platforms like TikTok

## Prerequisites
To run this program, you will need to have the FFmpeg library installed on your system. Here's how to install it for various platforms:

### Ubuntu / Debian
```sh
sudo apt-get install ffmpeg
```

### Arch
```sh
sudo pacman -S ffmpeg
```

### Mac
```sh
brew install ffmpeg
```

### Windows
Download the latest version of FFmpeg from the official website: ``https://ffmpeg.org/download.html``

## Building
To build the program, you will need to use CMake. Here are the steps:

Open a terminal and navigate to the root directory of the project.

- Create a build directory: ``mkdir src/build && cd src/build``
- Run CMake to generate the build files: ``cmake ..``
- Build the program: ``make``

## Running
To run the program, you will need to provide a video file as input. Here's how to do it:

- Navigate to the binary directory: ``cd bin``
- Run the program with a video file: ``./program path/to/video/file.mp4``
The program will process the video file and output the result to the console.

## Troubleshooting
If you encounter any issues while building or running the program, please refer to the documentation for FFmpeg or the CMake documentation. You can also check the issues section of this repository to see if anyone else has encountered similar issues.

## License
This project is licensed under the MIT License. See the LICENSE file for details.
