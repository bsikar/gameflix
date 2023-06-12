#include "frame_extractor.hpp"
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <video_path> <output_dir>"
              << std::endl;
    return 1;
  }

  try {
    FrameExtractor extractor(argv[1]);
    extractor.extract_frames(argv[2]);
    std::cout << "Frames extracted: " << extractor.frame_count << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
