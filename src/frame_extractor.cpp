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
  // Step 1: Retrieve the stream information from the video file
  if (avformat_open_input(&format_context, video_path.c_str(), nullptr,
                          nullptr) != 0) {
    throw std::runtime_error("[ERROR] Failed to open video file.");
  }

  // Step 2: Retrieve the stream information from the video file
  if (avformat_find_stream_info(format_context, nullptr) < 0) {
    avformat_close_input(&format_context);
    throw std::runtime_error("[ERROR] Failed to retrieve stream information.");
  }

  // Step 3: Find the video stream in the format context
  // (will throw error if bad)
  find_video_stream();

  // Step 4: Initialize the video codec for decoding frames
  // (will throw error if bad)
  init_video_codec();
}

// Destructor to clean up allocated resources
FrameExtractor::~FrameExtractor() {
  avcodec_free_context(&codec_context);
  avformat_close_input(&format_context);
}

void FrameExtractor::extract_frames(const std::string &output_dir) {
  AVPacket packet;
  // must be freed with av_frame_free();
  AVFrame *frame = av_frame_alloc();

  // Step 1: Read packets from the format context until the end of
  // the video stream is reached
  while (av_read_frame(format_context, &packet) >= 0) {
    // Step 2: Send packets to the codec context for decoding and
    // receive frames
    if (packet.stream_index == video_stream_index) {
      // get the packet to iter the frames
      avcodec_send_packet(codec_context, &packet);

      for (; avcodec_receive_frame(codec_context, frame) == 0; frame_count++) {
        // Step 3: Save each receive frame as an image file in the
        // output directory
        std::string frame_path =
            output_dir + "/frame_" + std::to_string(frame_count) + ".png";
        save_frame_as_image(frame, frame_path);
        std::cout << "[INFO] Processed " << frame_path << std::endl;
      }
    }
    // no need for the packet anymore
    av_packet_unref(&packet);
  }

  // no need for the frame anymore
  av_frame_free(&frame);
}

void FrameExtractor::find_video_stream() {
  for (unsigned int i = 0; i < format_context->nb_streams; i++) {
    const AVMediaType codec = format_context->streams[i]->codecpar->codec_type;
    if (codec == AVMEDIA_TYPE_VIDEO) {
      video_stream_index = i;
      break;
    }
  }

  if (video_stream_index == -1) {
    throw std::runtime_error("[ERROR] Failed to find video stream.");
  }
}

void FrameExtractor::init_video_codec() {
  // needs to be freed with avcodec_free_context();
  codec_context = avcodec_alloc_context3(nullptr);

  if (!codec_context) {
    throw std::runtime_error("[ERROR] Failed to allocated codec context.");
  }

  const auto *codecpars = format_context->streams[video_stream_index]->codecpar;
  const auto params_result =
      avcodec_parameters_to_context(codec_context, codecpars);

  if (params_result < 0) {
    avcodec_free_context(&codec_context);
    throw std::runtime_error("[ERROR] Failed to find video decoder.");
  }

  codec = const_cast<AVCodec *>(avcodec_find_decoder(codec_context->codec_id));
  const auto init_result = avcodec_open2(codec_context, codec, nullptr);

  if (init_result < 0) {
    avcodec_free_context(&codec_context);
    throw std::runtime_error("[ERROR] Failed to open video codec.");
  }
}

void FrameExtractor::save_frame_as_image(AVFrame *frame,
                                         const std::string &frame_path) {
  // Step 1: Find the PNG codec
  AVCodec *png_codec =
      const_cast<AVCodec *>(avcodec_find_encoder(AV_CODEC_ID_PNG));
  if (!png_codec) {
    throw std::runtime_error("[ERROR] Failed to find PNG codec.");
  }

  // Allocate and initialize the PNG codec context
  // needs to be freed with avcodec_free_context();
  AVCodecContext *png_codec_context = avcodec_alloc_context3(png_codec);
  if (!png_codec_context) {
    throw std::runtime_error("[ERROR] Failed to allocate PNG codec context.");
  }

  // Set the PNG codec parameters
  png_codec_context->width = frame->width;
  png_codec_context->height = frame->height;
  png_codec_context->pix_fmt = AV_PIX_FMT_RGBA;
  png_codec_context->time_base =
      format_context->streams[video_stream_index]->time_base;

  if (avcodec_open2(png_codec_context, png_codec, nullptr) < 0) {
    avcodec_free_context(&png_codec_context);
    throw std::runtime_error("[ERROR] Failed to open PNG codec.");
  }

  // Step 2: Create a temporary frame for the PNG conversion
  AVFrame *png_frame = av_frame_alloc();
  if (!png_frame) {
    avcodec_free_context(&png_codec_context);
    throw std::runtime_error("[ERROR] Failed to allocate PNG frame.");
  }
  png_frame->format = png_codec_context->pix_fmt;
  png_frame->width = png_codec_context->width;
  png_frame->height = png_codec_context->height;

  if (av_frame_get_buffer(png_frame, 0) < 0) {
    av_frame_free(&png_frame);
    avcodec_free_context(&png_codec_context);
    throw std::runtime_error("[ERROR] Failed to allocate PNG frame buffer.");
  }

  // Convert the input frame to the PNG pixel format
  SwsContext *sws_context = sws_getContext(
      frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
      png_codec_context->width, png_codec_context->height,
      png_codec_context->pix_fmt, 0, nullptr, nullptr, nullptr);

  if (!sws_context) {
    av_frame_free(&png_frame);
    avcodec_free_context(&png_codec_context);
    throw std::runtime_error(
        "[ERROR] Failed to create frame conversion context.");
  }

  sws_scale(sws_context, frame->data, frame->linesize, 0, frame->height,
            png_frame->data, png_frame->linesize);
  sws_freeContext(sws_context);

  // Step 3: Open the output file for writing
  std::ofstream output_file(frame_path, std::ios::binary);

  if (!output_file) {
    av_frame_free(&png_frame);
    avcodec_free_context(&png_codec_context);
    throw std::runtime_error("[ERROR] Failed to open output file.");
  }

  // Encode the PNG frame and write the data to the output file
  AVPacket *png_packet = av_packet_alloc();
  if (!png_packet) {
    av_frame_free(&png_frame);
    avcodec_free_context(&png_codec_context);
    throw std::runtime_error("[ERROR] Failed to allocate PNG packet.");
  }

  const auto frame_result = avcodec_send_frame(png_codec_context, png_frame);
  const auto packet_result =
      avcodec_receive_packet(png_codec_context, png_packet);

  if (frame_result >= 0 && packet_result >= 0) {
    output_file.write(reinterpret_cast<const char *>(png_packet->data),
                      png_packet->size);
  }

  // Clean up
  output_file.close();
  av_packet_unref(png_packet);
  av_packet_free(&png_packet);
  av_frame_free(&png_frame);
  avcodec_free_context(&png_codec_context);
}
