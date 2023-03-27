#include "frame_combiner.hpp"
#include "frame_extractor.hpp"
#include <algorithm>
#include <cxxopts.hpp>
#include <filesystem>
#include <iostream>
#include <string>

static const std::string VERSION = "0.1.0";
static const std::string AUTHOR = "Brighton Sikarskie";
static const std::string PROGRAM_NAME = "Gameflix";
static const std::string VIDEO_TMP_DIR = ".tmp/gameflix_video_path_tmp_dir";

int main(int argc, char **argv) {
  cxxopts::Options options(argv[0], PROGRAM_NAME);
  options.positional_help("<video_path_1> <video_path_2> <output_file_path>");
  // clang-format off
  options.add_options()
      ("h,help", "Print help information")
      ("v,version", "Print version information")
      ("video_path_1", "Path to the first video file", cxxopts::value<std::string>())
      ("video_path_2", "Path to the second video file", cxxopts::value<std::string>())
      ("output_file_path", "Path to the output file", cxxopts::value<std::string>());
  // clang-format on
  options.parse_positional(
      {"video_path_1", "video_path_2", "output_file_path"});

  try {
    // Remvoe the previous files
    std::filesystem::remove_all(VIDEO_TMP_DIR);
    std::cout << "[INFO] Removed previous tmp dir." << std::endl;
    // Create the directory and its parent directories if they don't exist
    std::filesystem::create_directories(VIDEO_TMP_DIR);
    std::cout << "[INFO] Created tmp dir." << std::endl;
  } catch (const std::filesystem::filesystem_error &e) {
    std::cout << "Failed to create directories: " << e.what() << std::endl;
  }

  try {
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
      std::cout << options.help() << std::endl;
      return 1;
    }

    if (result.count("version")) {
      std::cout << PROGRAM_NAME << " " << VERSION << std::endl;
      std::cout << "Author: " << AUTHOR << std::endl;
      return 1;
    }

    std::string video_path1 = result["video_path_1"].as<std::string>();
    std::string video_path2 = result["video_path_2"].as<std::string>();
    std::string output_file_path = result["output_file_path"].as<std::string>();

    // extract frames
    // TODO: ADD AUDIO
    FrameExtractor frame_extractor1(video_path1);
    FrameExtractor frame_extractor2(video_path2);
    int width = std::max(frame_extractor1.get_leading_zeros(),
                         frame_extractor2.get_leading_zeros());
    frame_extractor1.extract_frames(VIDEO_TMP_DIR, width);
    frame_extractor2.extract_frames(VIDEO_TMP_DIR, width);

    // stack frames
    // TODO

    // combine frames
    // TODO: ADD AUDIO
    FrameCombiner frame_combiner(VIDEO_TMP_DIR);
    frame_combiner.combine_frames_to_video(output_file_path);
  } catch (const std::exception &e) {
    std::cerr << "Error parsing options: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
