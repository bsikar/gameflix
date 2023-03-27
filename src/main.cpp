#include <iostream>
#include <string>

// Include the FFmpeg headers
extern "C" {
#include <libavformat/avformat.h>
}

int main(int argc, char **argv) {
  // Check that the user provided an input file
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
    return 1;
  }

  // Open input file
  AVFormatContext *format_ctx = nullptr;
  format_ctx = avformat_alloc_context();
  if (!format_ctx) {
    std::cerr << "Failed to allocate format context" << std::endl;
    return 1;
  }
  int ret = avformat_open_input(&format_ctx, argv[1], nullptr, nullptr);
  if (ret < 0) {
    // Use av_strerror() to print the error message directly
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(ret, errbuf, sizeof(errbuf));
    std::cerr << "Failed to open input file: " << errbuf << std::endl;
    avformat_free_context(format_ctx);
    return 1;
  }

  // Print some information about the input file
  av_dump_format(format_ctx, 0, argv[1], 0);

  // Clean up
  avformat_close_input(&format_ctx);
  avformat_free_context(format_ctx);

  return 0;
}
