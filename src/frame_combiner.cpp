#include "frame_combiner.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
}

FrameCombiner::FrameCombiner(const std::string &png_dir)
    : png_dir(png_dir), frames(), png_files(), format_context_(nullptr),
      codec_context_(nullptr), stream_(nullptr), frame_(nullptr) {}

FrameCombiner::~FrameCombiner() { cleanup_resources(); }

void FrameCombiner::combine_frames_to_video(
    const std::string &output_filename) {
  av_log_set_level(AV_LOG_DEBUG);

  setup_video_codec();

  open_output_file(output_filename);

  get_png_files_in_dir();

  convert_pngs_to_frames();

  process_frames();

  write_trailer();
}

void FrameCombiner::get_png_files_in_dir() {
  std::filesystem::path path(png_dir);

  if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
    std::cerr << "Invalid png_dir: " << png_dir << std::endl;
    return;
  }

  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    if (entry.is_regular_file() && entry.path().extension() == ".png") {
      png_files.push_back(entry.path().string());
    }
  }

  std::sort(png_files.begin(), png_files.end());
}

void FrameCombiner::write_trailer() {
  // Write the trailer
  av_write_trailer(format_context_);
}

void FrameCombiner::open_output_file(const std::string &output_filename) {
  // Create the format context
  if (avformat_alloc_output_context2(&format_context_, nullptr, nullptr,
                                     output_filename.c_str()) < 0) {
    std::cerr << "Failed to allocate the output format context." << std::endl;
    return;
  }

  // Find the video output format
  const AVOutputFormat *output_format = format_context_->oformat;

  // Create a new video stream
  stream_ = avformat_new_stream(format_context_, nullptr);
  if (!stream_) {
    std::cerr << "Failed to allocate the video stream." << std::endl;
    return;
  }

  // Set the codec parameters for the video stream
  AVCodecParameters *codec_parameters = stream_->codecpar;
  codec_parameters->codec_id = output_format->video_codec;
  codec_parameters->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_parameters->width = codec_context_->width;
  codec_parameters->height = codec_context_->height;
  codec_parameters->format = codec_context_->pix_fmt;

  // Open the output file
  if (avio_open(&format_context_->pb, output_filename.c_str(),
                AVIO_FLAG_WRITE) < 0) {
    std::cerr << "Failed to open the output file." << std::endl;

    return;
  }

  // Write the stream header
  if (avformat_write_header(format_context_, nullptr) < 0) {
    std::cerr << "Failed to write the stream header." << std::endl;

    return;
  }
}

void FrameCombiner::setup_video_codec() {
  // Find the video encoder
  const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec) {
    std::cerr << "Failed to find the video encoder." << std::endl;
    return;
  }

  // Create a new codec context
  codec_context_ = avcodec_alloc_context3(codec);
  if (!codec_context_) {
    std::cerr << "Failed to allocate the video codec context." << std::endl;
    return;
  }

  // Set the codec parameters
  codec_context_->bit_rate = 400000;
  // codec_context_->width = 1920;
  codec_context_->width = 1280;
  codec_context_->height = 1080;
  codec_context_->time_base = {1, 25};
  codec_context_->framerate = {25, 1};
  codec_context_->gop_size = 10;
  codec_context_->max_b_frames = 1;
  codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;

  // Open the codec
  if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
    std::cerr << "Failed to open the video codec." << std::endl;
    return;
  }
}

void FrameCombiner::cleanup_resources() {
  // Free the codec context
  avcodec_free_context(&codec_context_);

  // Close the output file
  if (format_context_ && format_context_->pb) {
    avio_close(format_context_->pb);
  }

  // Free the format context
  avformat_free_context(format_context_);

  // Free the frame
  av_frame_free(&frame_);
}

void FrameCombiner::process_frames() {
  // Set up the frame
  frame_ = av_frame_alloc();
  if (!frame_) {
    std::cerr << "Failed to allocate the video frame." << std::endl;
    return;
  }

  frame_->format = codec_context_->pix_fmt;
  frame_->width = codec_context_->width;
  frame_->height = codec_context_->height;

  // Allocate the frame buffer
  if (av_frame_get_buffer(frame_, 32) < 0) {
    std::cerr << "Failed to allocate the video frame buffer." << std::endl;
    return;
  }

  // Iterate over the frames and encode them
  for (unsigned int i = 0; i < frames.size(); ++i) {
    AVFrame *currentFrame = frames[i];

    // Convert the pixel format if necessary
    if (currentFrame->format != codec_context_->pix_fmt) {
      AVFrame *converted_frame = av_frame_alloc();
      if (!converted_frame) {
        std::cerr << "Failed to allocate the converted frame." << std::endl;
        return;
      }

      converted_frame->format = codec_context_->pix_fmt;
      converted_frame->width = currentFrame->width;
      converted_frame->height = currentFrame->height;

      if (av_frame_get_buffer(converted_frame, 32) < 0) {
        std::cerr << "Failed to allocate the converted frame buffer."
                  << std::endl;
        av_frame_free(&converted_frame);
        return;
      }

      SwsContext *swsContext = sws_getContext(
          currentFrame->width, currentFrame->height,

          static_cast<AVPixelFormat>(currentFrame->format), currentFrame->width,
          currentFrame->height,

          codec_context_->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);

      if (!swsContext) {
        std::cerr << "Failed to initialize the image converter." << std::endl;
        av_frame_free(&converted_frame);
        return;
      }

      sws_scale(swsContext, currentFrame->data, currentFrame->linesize, 0,
                currentFrame->height, converted_frame->data,
                converted_frame->linesize);

      sws_freeContext(swsContext);

      currentFrame = converted_frame;
    }

    // Set the current frame
    frame_->data[0] = currentFrame->data[0];
    frame_->data[1] = currentFrame->data[1];
    frame_->data[2] = currentFrame->data[2];

    frame_->linesize[0] = currentFrame->linesize[0];
    frame_->linesize[1] = currentFrame->linesize[1];
    frame_->linesize[2] = currentFrame->linesize[2];

    // Set the frame properties
    frame_->pts = i;

    // Encode and write the frame to the output file
    AVPacket *packet = av_packet_alloc();

    if (avcodec_send_frame(codec_context_, frame_) < 0) {
      std::cerr << "Error sending a frame to the codec." << std::endl;
      return;
    }

    while (avcodec_receive_packet(codec_context_, packet) == 0) {
      av_packet_rescale_ts(packet, codec_context_->time_base,
                           stream_->time_base);
      packet->stream_index = stream_->index;

      if (av_interleaved_write_frame(format_context_, packet) < 0) {
        std::cerr << "Error writing video frame." << std::endl;
        return;
      }

      av_packet_unref(packet);
    }
  }

  // Free the frame
  av_frame_free(&frame_);
}

void FrameCombiner::convert_pngs_to_frames() {
  unsigned int i = 0;
  std::cout << "here" << std::endl;

  for (const auto &png_file : png_files) {
    // Convert PNG to AVFrame
    AVFrame *frame = convert_png_to_av_frame(png_file);

    if (!frame) {
      std::cerr << "Failed to convert PNG to AVFrame for file: " << png_file
                << std::endl;
      continue;
    }

    // Rescale the frame if necessary
    if (frame->width != codec_context_->width ||
        frame->height != codec_context_->height) {
      AVFrame *rescaled_frame = av_frame_alloc();
      if (!rescaled_frame) {
        std::cerr << "Failed to allocate the rescaled frame." << std::endl;
        av_frame_free(&frame);
        continue;
      }

      rescaled_frame->format = codec_context_->pix_fmt;
      rescaled_frame->width = codec_context_->width;
      rescaled_frame->height = codec_context_->height;

      if (av_frame_get_buffer(rescaled_frame, 32) < 0) {
        std::cerr << "Failed to allocate the rescaled frame buffer."
                  << std::endl;
        av_frame_free(&rescaled_frame);
        av_frame_free(&frame);
        continue;
      }

      SwsContext *swsContext = sws_getContext(
          frame->width, frame->height,
          static_cast<AVPixelFormat>(frame->format), codec_context_->width,
          codec_context_->height, codec_context_->pix_fmt, SWS_BICUBIC, nullptr,
          nullptr, nullptr);

      if (!swsContext) {
        std::cerr << "Failed to initialize the image converter." << std::endl;
        av_frame_free(&rescaled_frame);
        av_frame_free(&frame);

        continue;
      }

      sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
                rescaled_frame->data, rescaled_frame->linesize);

      sws_freeContext(swsContext);

      // Free the original frame
      av_frame_free(&frame);

      // Set the rescaled frame
      frame = rescaled_frame;
    }

    // Set the frame properties
    frame->pts = i++;

    // Encode and write the frame to the output file
    AVPacket *packet = av_packet_alloc();

    if (avcodec_send_frame(codec_context_, frame) < 0) {
      std::cerr << "Error sending a frame to the codec." << std::endl;
      av_frame_free(&frame);
      continue;
    }

    while (avcodec_receive_packet(codec_context_, packet) == 0) {
      av_packet_rescale_ts(packet, codec_context_->time_base,
                           stream_->time_base);
      packet->stream_index = stream_->index;

      if (av_interleaved_write_frame(format_context_, packet) < 0) {
        std::cerr << "Error writing video frame." << std::endl;
        break;
      }

      av_packet_unref(packet);
    }

    // Free the frame and packet
    av_frame_free(&frame);
    av_packet_free(&packet);
  }
}

AVFrame *FrameCombiner::convert_png_to_av_frame(const std::string &file_path) {
  // Initialize the AVFormatContext
  AVFormatContext *format_context = nullptr;
  if (avformat_open_input(&format_context, file_path.c_str(), nullptr,
                          nullptr) != 0) {
    std::cerr << "Failed to open input file." << std::endl;
    return nullptr;
  }

  // Retrieve stream information
  if (avformat_find_stream_info(format_context, nullptr) < 0) {

    std::cerr << "Failed to retrieve stream information." << std::endl;
    avformat_close_input(&format_context);

    return nullptr;
  }

  // Find the video stream
  AVCodecParameters *codec_parameters = nullptr;
  int video_stream_index = av_find_best_stream(
      format_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (video_stream_index < 0) {
    std::cerr << "Failed to find video stream." << std::endl;

    avformat_close_input(&format_context);
    return nullptr;
  }

  codec_parameters = format_context->streams[video_stream_index]->codecpar;
  const AVCodec *codec = avcodec_find_decoder(codec_parameters->codec_id);
  if (!codec) {
    std::cerr << "Failed to find decoder." << std::endl;
    avformat_close_input(&format_context);

    return nullptr;
  }

  // Create a codec context and initialize it
  AVCodecContext *codec_context = avcodec_alloc_context3(codec);

  if (!codec_context) {
    std::cerr << "Failed to allocate codec context." << std::endl;
    avformat_close_input(&format_context);
    return nullptr;
  }

  if (avcodec_parameters_to_context(codec_context, codec_parameters) < 0) {
    std::cerr << "Failed to copy codec parameters to context." << std::endl;
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);

    return nullptr;
  }

  if (avcodec_open2(codec_context, codec, nullptr) < 0) {
    std::cerr << "Failed to open codec." << std::endl;
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
    return nullptr;
  }

  // Create an AVFrame to store the decoded frame
  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    std::cerr << "Failed to allocate frame." << std::endl;
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
    return nullptr;
  }

  // Read the packet and decode frames until the end of the file
  AVPacket packet;
  int frame_finished = 0;

  while (av_read_frame(format_context, &packet) >= 0) {
    if (packet.stream_index == video_stream_index) {
      if (avcodec_send_packet(codec_context, &packet) < 0) {
        std::cerr << "Failed to send packet to the decoder." << std::endl;
        break;
      }

      if (avcodec_receive_frame(codec_context, frame) == 0) {
        frame_finished = 1;

        break;
      }
    }
    av_packet_unref(&packet);
  }

  // Cleanup
  av_packet_unref(&packet);
  avcodec_free_context(&codec_context);
  avformat_close_input(&format_context);

  if (!frame_finished) {
    std::cerr << "Failed to decode frame." << std::endl;
    av_frame_free(&frame);
    return nullptr;
  }

  return frame;
}
