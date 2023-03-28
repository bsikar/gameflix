#include <iostream>
#include <vector>
// FFmpeg
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}

// I got the base of this code from a github issue comment, but I cannot find it
// anymore... I am going to change this code anyways later and their code was
// deprecated

int main(int argc, char **argv) {
  // Check if the correct number of command line arguments were provided
  if (argc < 2) {
    std::cout << "Usage: program <input_file>" << std::endl;
    return 1;
  }

  // Get the input file name from the command line argument
  const char *input_file_name = argv[1];

  // Set the log level to debug
  av_log_set_level(AV_LOG_DEBUG);

  // Open the input file context
  AVFormatContext *input_format_context = nullptr;
  int result = avformat_open_input(&input_format_context, input_file_name,
                                   nullptr, nullptr);

  // Check if the input file was opened successfully
  if (result < 0) {
    std::cerr << "Failed to open input file \"" << input_file_name
              << "\": error code=" << result;
    return 2;
  }

  // Retrieve information about the input streams
  result = avformat_find_stream_info(input_format_context, nullptr);

  // Check if the input stream information was retrieved successfully
  if (result < 0) {
    std::cerr << "Failed to retrieve input stream information: error code="
              << result;
    return 2;
  }

  // Find the primary video stream
  AVCodec *video_codec = nullptr;
  result = av_find_best_stream(input_format_context, AVMEDIA_TYPE_VIDEO, -1, -1,
                               &video_codec, 0);

  // Check if the primary video stream was found successfully
  if (result < 0) {
    std::cerr << "Failed to find primary video stream: error code=" << result;
    return 2;
  }

  // Get the index of the primary video stream
  const int video_stream_index = result;

  // Get a pointer to the primary video stream
  AVStream *video_stream = input_format_context->streams[video_stream_index];

  // Open the video decoder context
  AVCodecContext *video_decoder_context = avcodec_alloc_context3(video_codec);
  if (!video_decoder_context) {
    std::cerr << "Failed to allocate video decoder context";
    return 2;
  }
  result = avcodec_parameters_to_context(video_decoder_context,
                                         video_stream->codecpar);

  // Check if the video decoder context was opened successfully
  if (result < 0) {
    std::cerr << "Failed to open video decoder context: error code=" << result;
    return 2;
  }
  result = avcodec_open2(video_decoder_context, video_codec, nullptr);

  // Check if the video codec was opened successfully
  if (result < 0) {
    std::cerr << "Failed to open video codec: error code=" << result;
    return 2;
  }

  // print input video stream information
  std::cout << "Input file: " << input_file_name << "\n"
            << "Format: " << input_format_context->iformat->name << "\n"
            << "Video codec: " << video_codec->name << "\n"
            << "Resolution: " << video_stream->codecpar->width << 'x'
            << video_stream->codecpar->height << "\n"
            << "Frame rate: " << av_q2d(video_stream->avg_frame_rate)
            << " [fps]\n"
            << "Duration: "
            << av_rescale_q(video_stream->duration, video_stream->time_base,
                            {1, AV_TIME_BASE}) /
                   AV_TIME_BASE
            << " [sec]\n"
            << "Pixel format: "
            << av_get_pix_fmt_name(
                   static_cast<AVPixelFormat>(video_stream->codecpar->format))
            << "\n"
            << "Number of frames: " << video_stream->nb_frames << "\n"
            << std::flush;

  // clean up
  avformat_close_input(&input_format_context);
  avcodec_free_context(&video_decoder_context);

  return 0;
}
