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

void save_video_to_frames(AVCodecContext *video_decoder_context,
                          AVStream *video_stream,
                          AVFormatContext *input_format_context) {
  int ret, frame_count = 0;
  AVFrame *frame = av_frame_alloc();
  AVPacket *pkt = av_packet_alloc();
  if (!frame || !pkt) {
    fprintf(stderr, "Failed to allocate frame or packet\n");
    exit(1);
  }

  // Initialize the scaler context to convert video frames to RGB
  SwsContext *sws_ctx = sws_getContext(
      video_decoder_context->width, video_decoder_context->height,
      video_decoder_context->pix_fmt, video_decoder_context->width,
      video_decoder_context->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, NULL, NULL,
      NULL);
  if (!sws_ctx) {
    fprintf(stderr, "Failed to initialize scaler context\n");
    exit(1);
  }

  // Open the output file
  char output_filename[1024];
  snprintf(output_filename, sizeof(output_filename), "frame-%d.ppm",
           frame_count);
  FILE *output_file = fopen(output_filename, "wb");
  if (!output_file) {
    fprintf(stderr, "Failed to open output file\n");
    exit(1);
  }

  // Write the PPM header to the output file
  fprintf(output_file, "P6\n%d %d\n255\n", video_decoder_context->width,
          video_decoder_context->height);

  while (av_read_frame(input_format_context, pkt) >= 0) {
    if (pkt->stream_index != video_stream->index) {
      av_packet_unref(pkt);
      continue;
    }

    ret = avcodec_send_packet(video_decoder_context, pkt);
    if (ret < 0) {
      fprintf(stderr, "Error sending a packet for decoding\n");
      exit(1);
    }

    while (ret >= 0) {
      ret = avcodec_receive_frame(video_decoder_context, frame);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        break;
      else if (ret < 0) {
        fprintf(stderr, "Error during decoding\n");
        exit(1);
      }

      // Convert the frame to RGB and save it to file
      uint8_t *rgb_buffer = (uint8_t *)malloc(
          video_decoder_context->width * video_decoder_context->height * 3);
      if (!rgb_buffer) {
        fprintf(stderr, "Failed to allocate RGB buffer\n");
        exit(1);
      }
      int linesize = video_decoder_context->width * 3;
      sws_scale(sws_ctx, frame->data, frame->linesize, 0,
                video_decoder_context->height, &rgb_buffer, &linesize);

      // Write the RGB buffer to the output file
      fwrite(rgb_buffer, 1,
             video_decoder_context->width * video_decoder_context->height * 3,
             output_file);
      fflush(output_file);

      // Increment frame count and update output filename
      frame_count++;
      snprintf(output_filename, sizeof(output_filename), "frame-%d.ppm",
               frame_count);

      // Close the current output file and open the new one
      fclose(output_file);
      output_file = fopen(output_filename, "wb");

      // Write the PPM header to the new output file
      fprintf(output_file, "P6\n%d %d\n255\n", video_decoder_context->width,
              video_decoder_context->height);

      // Free the RGB buffer
      free(rgb_buffer);
    }

    av_packet_unref(pkt);
  }

  // Close the output file and free memory
  fclose(output_file);
  av_frame_free(&frame);
  av_packet_free(&pkt);
  sws_freeContext(sws_ctx);
}

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
  std::string file_format_name = input_format_context->iformat->name;
  std::string codec_name = video_codec->name;
  int width = video_stream->codecpar->width;
  int height = video_stream->codecpar->height;
  int frame_rate = av_q2d(video_stream->avg_frame_rate);
  int duration = av_rescale_q(video_stream->duration, video_stream->time_base,
                              {1, AV_TIME_BASE}) /
                 AV_TIME_BASE;
  std::string pixel_format_name = av_get_pix_fmt_name(
      static_cast<AVPixelFormat>(video_stream->codecpar->format));
  int frame_count = video_stream->nb_frames;

  std::cout << "Input file: " << input_file_name << std::endl
            << "Format: " << file_format_name << std::endl
            << "Video codec: " << video_codec << std::endl
            << "Resolution: " << width << 'x' << height << std::endl
            << "Frame rate: " << frame_rate << " [fps]" << std::endl
            << "Duration: " << duration << " [sec]" << std::endl
            << "Pixel format: " << pixel_format_name << std::endl
            << "Number of frames: " << frame_count << std::endl
            << std::flush;

  save_video_to_frames(video_decoder_context, video_stream,
                       input_format_context);

  // Free the allocated resources
  avcodec_free_context(&video_decoder_context);
  avformat_close_input(&input_format_context);

  return 0;
}
