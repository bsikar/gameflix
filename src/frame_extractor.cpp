#include "frame_extractor.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

FrameExtractor::FrameExtractor(const std::string &video_path)
    : format_context(nullptr), codec_context(nullptr), codec(nullptr),
      video_stream_index(-1), frame_count(0) {
  // STEP 1: Retrieve the stream information from the video file
  if (avformat_open_input(&format_context, video_path.c_str(), nullptr,
                          nullptr) != 0) {
    std::cerr << "Failed to open video file." << std::endl;
    return;
  }

  // STEP 2: Retrieve the stream information from the video file
  if (avformat_find_stream_info(format_context, nullptr) < 0) {
    avformat_close_input(&format_context);
    std::cerr << "Failed to retrieve stream information." << std::endl;
    return;
  }

  // STEP 3: Find the video stream in the format context
  find_video_stream();

  // STEP 4: Initialize the video codec for decoding frames
  init_video_codec();
}

// Destructor to clean up allocated resources
FrameExtractor::~FrameExtractor() {
  avcodec_free_context(&codec_context);
  avformat_close_input(&format_context);
}

int FrameExtractor::get_leading_zeros() {
  AVPacket packet;
  AVFrame *frame = av_frame_alloc();

  // STEP 1: Calculate the total number of frames
  int total_frames = 0;
  while (av_read_frame(format_context, &packet) >= 0) {
    if (packet.stream_index == video_stream_index) {
      total_frames += 1;
    }

    av_packet_unref(&packet);
  }
  av_seek_frame(format_context, video_stream_index, 0, AVSEEK_FLAG_BACKWARD);

  // STEP 2: Determine the width for leading zeros
  int width = 1;

  int temp = total_frames;
  while (temp /= 10) {
    width += 1;
  }

  // STEP 3: Cleanup
  av_frame_free(&frame);

  return width;
}

void FrameExtractor::extract_frames(const std::string &output_dir, int width) {
  AVPacket packet;
  AVFrame *frame = av_frame_alloc();

  // STEP 1: Read packets from the format context until the end of
  // the video stream is reached
  int frame_count = 0;
  while (av_read_frame(format_context, &packet) >= 0) {
    // STEP 2: Send packets to the codec context for decoding and
    // receive frames
    if (packet.stream_index == video_stream_index) {
      avcodec_send_packet(codec_context, &packet);

      for (; avcodec_receive_frame(codec_context, frame) == 0; frame_count++) {
        // STEP 3: Save each received frame as an image file in the
        // output directory with leading zeros in the filename
        std::stringstream frame_path_ss;
        frame_path_ss << output_dir << "/frame_" << std::setfill('0')
                      << std::setw(width) << frame_count << "_" << this
                      << ".png";

        std::string frame_path = frame_path_ss.str();

        save_frame_as_image(frame, frame_path);
        std::cout << "[INFO] Processed " << frame_path << std::endl;
      }
    }
    av_packet_unref(&packet);
  }

  av_frame_free(&frame);
}

void FrameExtractor::find_video_stream() {
  // STEP 1: Iterate through each stream in the format context
  for (unsigned int i = 0; i < format_context->nb_streams; i++) {
    // STEP 2: Get the codec type of the current stream
    const AVMediaType codec = format_context->streams[i]->codecpar->codec_type;

    // STEP 3: Check if the codec type is video
    if (codec == AVMEDIA_TYPE_VIDEO) {
      // STEP 4: Set the video stream index and exit the loop
      video_stream_index = i;
      return;
    }
  }

  // STEP 5: Check if the video stream index was found
  if (video_stream_index == -1) {
    std::cerr << "Failed to find video stream." << std::endl;
    return;
  }
}

void FrameExtractor::init_video_codec() {
  // STEP 1: Allocate the codec context
  codec_context = avcodec_alloc_context3(nullptr);

  if (!codec_context) {
    std::cerr << "Failed to allocate codec context." << std::endl;
    return;
  }

  // STEP 2: Copy codec parameters to the codec context
  const auto *codecpars = format_context->streams[video_stream_index]->codecpar;
  const auto params_result =
      avcodec_parameters_to_context(codec_context, codecpars);

  if (params_result < 0) {
    avcodec_free_context(&codec_context);
    std::cerr << "Failed to copy codec parameters to context." << std::endl;
    return;
  }

  // STEP 3: Find the video decoder codec
  codec = const_cast<AVCodec *>(avcodec_find_decoder(codec_context->codec_id));

  if (!codec) {

    avcodec_free_context(&codec_context);
    std::cerr << "Failed to find video decoder." << std::endl;
    return;
  }

  // STEP 4: Open the video codec

  const auto init_result = avcodec_open2(codec_context, codec, nullptr);

  if (init_result < 0) {
    avcodec_free_context(&codec_context);
    std::cerr << "Failed to open video codec." << std::endl;
    return;
  }
}

void FrameExtractor::save_frame_as_image(AVFrame *frame,
                                         const std::string &frame_path) {
  // STEP 1: Find the PNG codec
  AVCodec *png_codec =
      const_cast<AVCodec *>(avcodec_find_encoder(AV_CODEC_ID_PNG));
  if (!png_codec) {
    std::cerr << "Failed to find PNG codec." << std::endl;
    return;
  }

  // Allocate and initialize the PNG codec context
  AVCodecContext *png_codec_context =
      initialize_png_codec_context(png_codec, frame);

  // STEP 2: Create a temporary frame for the PNG conversion
  AVFrame *png_frame = create_png_frame(png_codec_context);

  // Convert the input frame to the PNG pixel format
  convert_frame_to_png(frame, png_frame, png_codec_context);

  // STEP 3: Open the output file for writing
  std::ofstream output_file(frame_path, std::ios::binary);
  if (!output_file) {
    std::cerr << "Failed to open output file." << std::endl;
    av_frame_free(&png_frame);
    avcodec_free_context(&png_codec_context);
    return;
  }

  // Encode the PNG frame and write the data to the output file
  encode_png_frame(png_codec_context, png_frame, output_file);

  // Clean up
  output_file.close();
  av_frame_free(&png_frame);
  avcodec_free_context(&png_codec_context);
}

AVCodecContext *FrameExtractor::initialize_png_codec_context(AVCodec *png_codec,
                                                             AVFrame *frame) {
  // STEP 1: Allocate and initialize the PNG codec context
  AVCodecContext *png_codec_context = avcodec_alloc_context3(png_codec);
  if (!png_codec_context) {
    std::cerr << "Failed to allocate PNG codec context." << std::endl;
    return nullptr;
  }

  // STEP 2: Set the PNG codec parameters
  png_codec_context->width = frame->width;
  png_codec_context->height = frame->height;
  png_codec_context->pix_fmt = AV_PIX_FMT_RGBA;
  png_codec_context->time_base =
      format_context->streams[video_stream_index]->time_base;

  if (avcodec_open2(png_codec_context, png_codec, nullptr) < 0) {
    std::cerr << "Failed to open PNG codec." << std::endl;
    avcodec_free_context(&png_codec_context);
    return nullptr;
  }

  return png_codec_context;
}

AVFrame *FrameExtractor::create_png_frame(AVCodecContext *png_codec_context) {
  // STEP 1: Create a temporary frame for the PNG conversion
  AVFrame *png_frame = av_frame_alloc();
  if (!png_frame) {
    std::cerr << "Failed to allocate PNG frame." << std::endl;
    avcodec_free_context(&png_codec_context);
    return nullptr;
  }

  // STEP 2: Set the PNG frame parameters
  png_frame->format = png_codec_context->pix_fmt;
  png_frame->width = png_codec_context->width;
  png_frame->height = png_codec_context->height;

  // STEP 3: Allocate the PNG frame buffer
  if (av_frame_get_buffer(png_frame, 0) < 0) {
    std::cerr << "Failed to allocate PNG frame buffer." << std::endl;
    av_frame_free(&png_frame);
    avcodec_free_context(&png_codec_context);
    return nullptr;
  }

  return png_frame;
}

void FrameExtractor::convert_frame_to_png(AVFrame *frame, AVFrame *png_frame,
                                          AVCodecContext *png_codec_context) {
  // STEP 1: Create the frame conversion context
  SwsContext *sws_context = sws_getContext(
      frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
      png_codec_context->width, png_codec_context->height,
      png_codec_context->pix_fmt, 0, nullptr, nullptr, nullptr);

  if (!sws_context) {
    std::cerr << "Failed to create frame conversion context." << std::endl;
    av_frame_free(&png_frame);
    avcodec_free_context(&png_codec_context);
    return;
  }

  // STEP 2: Perform the frame conversion
  sws_scale(sws_context, frame->data, frame->linesize, 0, frame->height,
            png_frame->data, png_frame->linesize);

  // STEP 3: Free the frame conversion context
  sws_freeContext(sws_context);
}

void FrameExtractor::encode_png_frame(AVCodecContext *png_codec_context,
                                      AVFrame *png_frame,
                                      std::ofstream &output_file) {
  // STEP 1: Allocate the PNG packet
  AVPacket *png_packet = av_packet_alloc();
  if (!png_packet) {
    std::cerr << "Failed to allocate PNG packet." << std::endl;
    av_frame_free(&png_frame);
    avcodec_free_context(&png_codec_context);
    return;
  }

  // STEP 2: Send the PNG frame to the PNG codec
  const auto frame_result = avcodec_send_frame(png_codec_context, png_frame);

  // STEP 3: Receive the encoded PNG packet from the PNG codec
  const auto packet_result =
      avcodec_receive_packet(png_codec_context, png_packet);

  // STEP 4: Write the packet data to the output file
  if (frame_result >= 0 && packet_result >= 0) {
    output_file.write(reinterpret_cast<const char *>(png_packet->data),
                      png_packet->size);
  }

  // STEP 5: Cleanup
  av_packet_unref(png_packet);
  av_packet_free(&png_packet);
}
