#include <fstream>
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

int save_video_to_frames(AVCodecContext *video_decoder_context,
                         AVStream *video_stream,
                         AVFormatContext *input_format_context) {
  // Initialize variables for return value and frame count
  int result, frame_count = 0;

  // Allocate memory for AVFrame and AVPacket pointers
  AVFrame *frame_ptr = av_frame_alloc();
  AVPacket *packet_ptr = av_packet_alloc();

  // Check if memory allocation was successful
  if (!frame_ptr || !packet_ptr) {
    std::cerr << "Failed to allocate frame or packet" << std::endl;
    return 1;
  }

  // Initialize the scaler context to convert video frames to RGB using
  // video_decoder_context settings
  SwsContext *scaler_context_ptr = sws_getContext(
      video_decoder_context->width, video_decoder_context->height,
      video_decoder_context->pix_fmt, video_decoder_context->width,
      video_decoder_context->height, AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr,
      nullptr, nullptr);

  if (!scaler_context_ptr) {
    std::cerr << "Failed to initialize scaler context" << std::endl;
    return 1;
  }

  // Open the output file
  std::string output_filename = "frame-" + std::to_string(frame_count) + ".ppm";
  std::ofstream output_file(output_filename, std::ios::binary);
  if (!output_file) {
    std::cerr << "Failed to open output file\n";
    exit(1);
  }

  // Write the PPM header to the output file
  output_file << "P6\n"
              << video_decoder_context->width << " "
              << video_decoder_context->height << "\n255\n";

  // Read each packet from the input file until there are no more packets
  while (av_read_frame(input_format_context, packet_ptr) >= 0) {
    // Check if the packet is from the video stream
    if (packet_ptr->stream_index != video_stream->index) {
      av_packet_unref(packet_ptr);
      continue;
    }

    // Send the packet for decoding
    result = avcodec_send_packet(video_decoder_context, packet_ptr);
    if (result < 0) {
      std::cerr << "Error sending a packet for decoding" << std::endl;
      return 1;
    }

    // Decode the video frames
    while (result >= 0) {
      result = avcodec_receive_frame(video_decoder_context, frame_ptr);
      // If the decoder needs more input data to output a frame or if the end of
      // the stream is reached, break out of the loop
      if (result == AVERROR(EAGAIN) || result == AVERROR_EOF) {
        break;
        // If there was an error during decoding, print an error message and
        // exit the program
      } else if (result < 0) {
        std::cerr << "Error during decoding" << std::endl;
        return 1;
      }

      // Convert the frame to RGB and save it to file
      // Allocate a vector to hold the RGB buffer
      std::vector<uint8_t> rgb_buffer(video_decoder_context->width *
                                      video_decoder_context->height * 3);

      // Prepare an array of pointers to the destination data and an array of
      // destination strides, which specifies the number of bytes between
      // successive rows of the output image. In this case, there is only one
      // row, so there is only one stride value.
      uint8_t *dest_data[1] = {rgb_buffer.data()};
      int dest_stride[1] = {video_decoder_context->width * 3};

      // Call sws_scale() to convert the input frame to RGB format and store the
      // result in the destination buffer.
      // scaler_context_ptr is a pointer to a SwsContext structure that contains
      // various parameters for the scaling operation, such as the input and
      // output image sizes and formats. frame_ptr is a pointer to an AVFrame
      // structure that contains the input image data.
      sws_scale(scaler_context_ptr, frame_ptr->data, frame_ptr->linesize, 0,
                video_decoder_context->height, dest_data, dest_stride);

      // Write the RGB buffer to the output file
      // output_file is a std::ofstream object that represents the output file->
      // reinterpret_cast<char *>(rgb_buffer.data()) is used to cast the
      // unsigned char pointer to a char pointer, which is the type expected by
      // the write() function. video_decoder_context.width *
      // video_decoder_context.height * 3 is the size of the RGB buffer in
      // bytes.
      output_file.write(reinterpret_cast<char *>(rgb_buffer.data()),
                        video_decoder_context->width *
                            video_decoder_context->height * 3);

      // Increment frame count and update output filename
      frame_count++;
      output_filename = "frame-" + std::to_string(frame_count) + ".ppm";

      // Close the current output file and open the new one
      output_file.close();
      output_file.open(output_filename, std::ios::binary);

      // Write the PPM header to the new output file
      output_file << "P6\n"
                  << video_decoder_context->width << " "
                  << video_decoder_context->height << "\n255\n";

      // Free the RGB buffer
      rgb_buffer.clear();
    }

    // Unreference the packet after it has been processed to release the
    // allocated memory
    av_packet_unref(packet_ptr);
  }

  // Close the output file and free memory
  output_file.close();

  // delete the last file saved
  std::remove(output_filename.c_str());

  av_frame_free(&frame_ptr);
  av_packet_free(&packet_ptr);
  sws_freeContext(scaler_context_ptr);

  return 0;
}

void print_video_stream_info(AVFormatContext *input_format_context,
                             AVCodec *video_codec, AVStream *video_stream) {
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

  std::cout << "Input file format: " << file_format_name << std::endl
            << "Video codec name: " << codec_name << std::endl
            << "Video resolution: " << width << 'x' << height << std::endl
            << "Video frame rate: " << frame_rate << " fps" << std::endl
            << "Video duration: " << duration << " sec" << std::endl
            << "Video pixel format: " << pixel_format_name << std::endl
            << "Number of video frames: " << frame_count << std::endl;
}

AVCodecContext *open_video_decoder_context(AVCodec *video_codec,
                                           AVStream *video_stream) {
  AVCodecContext *video_decoder_context = avcodec_alloc_context3(video_codec);
  if (!video_decoder_context) {
    std::cerr << "Failed to allocate video decoder context" << std::endl;
    return nullptr;
  }

  int result = avcodec_parameters_to_context(video_decoder_context,
                                             video_stream->codecpar);
  if (result < 0) {
    std::cerr << "Failed to copy codec parameters to video decoder context: "
                 "error code="
              << result << std::endl;
    avcodec_free_context(&video_decoder_context);
    return nullptr;
  }

  result = avcodec_open2(video_decoder_context, video_codec, nullptr);
  if (result < 0) {
    std::cerr << "Failed to open video codec: error code=" << result
              << std::endl;
    avcodec_free_context(&video_decoder_context);
    return nullptr;
  }

  return video_decoder_context;
}

int main(int argc, char **argv) {
  // Check if the correct number of command line arguments were provided
  if (argc < 2) {
    std::cout << "Usage: program <input_file>" << std::endl;
    return 1;
  }

  // Get the input file name from the command line argument
  std::string input_file_name = argv[1];

  // Set the log level to debug
  av_log_set_level(AV_LOG_DEBUG);

  // Open the input file context
  AVFormatContext *input_format_context = nullptr;
  int result = avformat_open_input(&input_format_context,
                                   input_file_name.c_str(), nullptr, nullptr);
  if (result < 0) {
    std::cerr << "Failed to open input file \"" << input_file_name
              << "\": error code=" << result << std::endl;
    return 2;
  }

  // Retrieve information about the input streams
  result = avformat_find_stream_info(input_format_context, nullptr);
  if (result < 0) {
    std::cerr << "Failed to retrieve input stream information: error code="
              << result << std::endl;
    return 2;
  }

  // Find the primary video stream
  AVCodec *video_codec = nullptr;
  result = av_find_best_stream(input_format_context, AVMEDIA_TYPE_VIDEO, -1, -1,
                               &video_codec, 0);
  if (result < 0) {
    std::cerr << "Failed to find primary video stream: error code=" << result
              << std::endl;
    return 2;
  }

  // Get the index of the primary video stream and a pointer to the stream
  const int video_stream_index = result;
  AVStream *video_stream = input_format_context->streams[video_stream_index];

  // Open the video decoder context and check for errors
  AVCodecContext *video_decoder_context =
      open_video_decoder_context(video_codec, video_stream);
  if (!video_decoder_context) {
    avformat_close_input(&input_format_context);
    return 2;
  }

  // Print input video stream information
  print_video_stream_info(input_format_context, video_codec, video_stream);

  // Save video to frames and check for errors
  result = save_video_to_frames(video_decoder_context, video_stream,
                                input_format_context);
  if (result < 0) {
    avcodec_free_context(&video_decoder_context);
    avformat_close_input(&input_format_context);
    return 2;
  }

  // Free allocated resources and exit
  avcodec_free_context(&video_decoder_context);
  avformat_close_input(&input_format_context);
  return 0;
}
