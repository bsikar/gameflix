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
  // STEP 1: Set up video codec
  setup_video_codec();

  // STEP 2: Open the output file
  open_output_file(output_filename);

  // STEP 3: Get PNG files in the directory
  get_png_files_in_dir();

  // STEP 4: Convert PNGs to frames
  convert_pngs_to_frames();

  // STEP 5: Process the frames
  process_frames();

  // STEP 6: Write the trailer
  write_trailer();
}

void FrameCombiner::get_png_files_in_dir() {
  std::filesystem::path path(png_dir);

  // STEP 1: Check if the directory exists and is a valid directory
  if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path)) {
    std::cerr << "Invalid png_dir: " << png_dir << std::endl;
    return;
  }

  // STEP 2: Iterate through the directory entries
  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    // STEP 3: Check if the entry is a regular file with the ".png" extension
    if (entry.is_regular_file() && entry.path().extension() == ".png") {
      // STEP 4: Add the file path to the png_files vector
      png_files.push_back(entry.path().string());
    }
  }

  // STEP 5: Sort the png_files vector
  std::sort(png_files.begin(), png_files.end());
}

void FrameCombiner::write_trailer() { av_write_trailer(format_context_); }

void FrameCombiner::open_output_file(const std::string &output_filename) {
  // STEP 1: Create the format context
  if (avformat_alloc_output_context2(&format_context_, nullptr, nullptr,
                                     output_filename.c_str()) < 0) {
    std::cerr << "Failed to allocate the output format context." << std::endl;
    return;
  }

  // STEP 2: Find the video output format
  const AVOutputFormat *output_format = format_context_->oformat;

  // STEP 3: Create a new video stream
  stream_ = avformat_new_stream(format_context_, nullptr);
  if (!stream_) {
    std::cerr << "Failed to allocate the video stream." << std::endl;
    return;
  }

  // STEP 4: Set the codec parameters for the video stream
  AVCodecParameters *codec_parameters = stream_->codecpar;
  codec_parameters->codec_id = output_format->video_codec;
  codec_parameters->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_parameters->width = codec_context_->width;
  codec_parameters->height = codec_context_->height;
  codec_parameters->format = codec_context_->pix_fmt;

  // STEP 5: Open the output file
  if (avio_open(&format_context_->pb, output_filename.c_str(),
                AVIO_FLAG_WRITE) < 0) {
    std::cerr << "Failed to open the output file." << std::endl;
    return;
  }

  // STEP 6: Write the stream header
  if (avformat_write_header(format_context_, nullptr) < 0) {
    std::cerr << "Failed to write the stream header." << std::endl;
    return;
  }
}

void FrameCombiner::setup_video_codec() {
  // STEP 1: Find the video encoder
  const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec) {
    std::cerr << "Failed to find the video encoder." << std::endl;
    return;
  }

  // STEP 2: Create a new codec context
  codec_context_ = avcodec_alloc_context3(codec);
  if (!codec_context_) {
    std::cerr << "Failed to allocate the video codec context." << std::endl;
    return;
  }

  // STEP 3: Set the codec parameters
  codec_context_->bit_rate = 8000000;
  codec_context_->width = 1920;
  codec_context_->height = 1080;
  codec_context_->time_base = {1, 30};
  codec_context_->framerate = {30, 1};
  codec_context_->gop_size = 10;
  codec_context_->max_b_frames = 1;
  codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;

  // STEP 4: Open the codec
  if (avcodec_open2(codec_context_, codec, nullptr) < 0) {
    std::cerr << "Failed to open the video codec." << std::endl;
    return;
  }
}

void FrameCombiner::cleanup_resources() {
  // STEP 1: Free the codec context
  avcodec_free_context(&codec_context_);

  // STEP 2: Close the output file
  if (format_context_ && format_context_->pb) {
    avio_close(format_context_->pb);
  }

  // STEP 3: Free the format context
  avformat_free_context(format_context_);

  // STEP 4: Free the frame
  av_frame_free(&frame_);
}

void FrameCombiner::process_frames() {
  // STEP 1: Set up the frame
  AVFrame *frame = setup_frame();
  if (!frame) {
    return;
  }

  // STEP 2: Iterate over the frames and encode them
  for (unsigned int i = 0; i < frames.size(); ++i) {
    AVFrame *currentFrame = frames[i];

    // STEP 3: Convert the pixel format if necessary
    if (currentFrame->format != codec_context_->pix_fmt) {
      AVFrame *converted_frame = convert_pixel_format(currentFrame);
      if (!converted_frame) {
        av_frame_free(&frame);
        return;
      }

      currentFrame = converted_frame;
    }

    // STEP 4: Set the current frame
    set_current_frame(frame, currentFrame);

    // STEP 5: Set the frame properties
    frame->pts = i;

    // STEP 6: Encode and write the frame to the output file
    if (!encode_and_write_frame(frame)) {
      av_frame_free(&frame);
      return;
    }
  }

  // STEP 7: Free the frame
  av_frame_free(&frame);
}

AVFrame *FrameCombiner::convert_pixel_format(AVFrame *frame) {
  // STEP 1: Allocate memory for the converted frame
  AVFrame *converted_frame = av_frame_alloc();
  if (!converted_frame) {
    std::cerr << "Failed to allocate the converted frame." << std::endl;
    return nullptr;
  }

  // STEP 2: Set the converted frame properties
  converted_frame->format = codec_context_->pix_fmt;
  converted_frame->width = frame->width;
  converted_frame->height = frame->height;

  // STEP 3: Allocate the converted frame buffer
  if (av_frame_get_buffer(converted_frame, 32) < 0) {
    std::cerr << "Failed to allocate the converted frame buffer." << std::endl;
    av_frame_free(&converted_frame);
    return nullptr;
  }

  // STEP 4: Perform pixel format conversion
  SwsContext *swsContext = sws_getContext(
      frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
      frame->width, frame->height, codec_context_->pix_fmt, SWS_BICUBIC,
      nullptr, nullptr, nullptr);

  if (!swsContext) {
    std::cerr << "Failed to initialize the image converter." << std::endl;
    av_frame_free(&converted_frame);
    return nullptr;
  }

  sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height,
            converted_frame->data, converted_frame->linesize);

  sws_freeContext(swsContext);

  return converted_frame;
}

AVFrame *FrameCombiner::setup_frame() {
  // STEP 1: Allocate memory for the frame
  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    std::cerr << "Failed to allocate the video frame." << std::endl;
    return nullptr;
  }

  // STEP 2: Set frame properties
  frame->format = codec_context_->pix_fmt;
  frame->width = codec_context_->width;
  frame->height = codec_context_->height;

  // STEP 3: Allocate the frame buffer
  if (av_frame_get_buffer(frame, 32) < 0) {
    std::cerr << "Failed to allocate the video frame buffer." << std::endl;
    av_frame_free(&frame);
    return nullptr;
  }

  return frame;
}

void FrameCombiner::set_current_frame(AVFrame *frame, AVFrame *currentFrame) {
  // STEP 1: Set the frame data
  frame->data[0] = currentFrame->data[0];
  frame->data[1] = currentFrame->data[1];
  frame->data[2] = currentFrame->data[2];

  // STEP 2: Set the frame linesize
  frame->linesize[0] = currentFrame->linesize[0];
  frame->linesize[1] = currentFrame->linesize[1];
  frame->linesize[2] = currentFrame->linesize[2];
}

void FrameCombiner::convert_pngs_to_frames() {
  unsigned int i = 0;
  for (const auto &png_file : png_files) {

    // STEP 1: Convert PNG to AVFrame
    AVFrame *frame = convert_png_to_av_frame(png_file);

    if (!frame) {
      std::cerr << "Failed to convert PNG to AVFrame for file: " << png_file
                << std::endl;
      continue;
    }

    // STEP 2: Rescale the frame if necessary
    frame = rescale_frame_if_necessary(frame);
    if (!frame) {
      continue;
    }

    // STEP 3: Set the frame properties
    frame->pts = i++;

    // STEP 4: Encode and write the frame to the output file
    if (!encode_and_write_frame(frame)) {
      av_frame_free(&frame);
      continue;
    }

    // Free the frame
    av_frame_free(&frame);
  }
}

AVFrame *FrameCombiner::rescale_frame_if_necessary(AVFrame *frame) {
  // STEP 1: Check if the frame size matches
  if (!is_frame_size_matching(frame)) {
    // STEP 2: Allocate a rescaled frame
    AVFrame *rescaled_frame = allocate_rescaled_frame();
    if (!rescaled_frame) {
      // Failed to allocate the rescaled frame
      std::cerr << "Failed to allocate the rescaled frame." << std::endl;
      av_frame_free(&frame);
      return nullptr;
    }

    // STEP 3: Initialize the rescaled frame
    if (!init_rescaled_frame(rescaled_frame)) {
      // Failed to initialize the rescaled frame
      std::cerr << "Failed to initialize the rescaled frame." << std::endl;
      av_frame_free(&rescaled_frame);
      av_frame_free(&frame);
      return nullptr;
    }

    // STEP 4: Scale the frame
    if (!scale_frame(frame, rescaled_frame)) {
      // Failed to scale the frame
      std::cerr << "Failed to scale the frame." << std::endl;
      av_frame_free(&rescaled_frame);
      av_frame_free(&frame);

      return nullptr;
    }

    // STEP 5: Free the original frame
    av_frame_free(&frame);

    // STEP 6: Set the rescaled frame
    frame = rescaled_frame;
  }

  return frame;
}

bool FrameCombiner::encode_and_write_frame(AVFrame *frame) {
  // STEP 1: Allocate a packet for the encoded frame
  AVPacket *packet = av_packet_alloc();

  // STEP 2: Send the frame to the codec for encoding
  if (avcodec_send_frame(codec_context_, frame) < 0) {
    // Error sending the frame to the codec
    std::cerr << "Error sending a frame to the codec." << std::endl;
    av_frame_free(&frame);
    av_packet_free(&packet);
    return false;
  }

  // STEP 3: Receive and write packets until no more packets are available
  while (avcodec_receive_packet(codec_context_, packet) == 0) {
    // Rescale the packet's timestamp
    av_packet_rescale_ts(packet, codec_context_->time_base, stream_->time_base);
    // Set the packet's stream index
    packet->stream_index = stream_->index;

    // Write the packet to the output file
    if (av_interleaved_write_frame(format_context_, packet) < 0) {
      // Error writing the video frame
      std::cerr << "Error writing video frame." << std::endl;
      av_packet_unref(packet);
      av_frame_free(&frame);
      av_packet_free(&packet);
      return false;
    }

    // Free the packet for reuse
    av_packet_unref(packet);
  }

  // STEP 4: Free the packet
  av_packet_free(&packet);
  return true;
}

bool FrameCombiner::is_frame_size_matching(const AVFrame *frame) const {
  return (frame->width == codec_context_->width &&
          frame->height == codec_context_->height);
}

AVFrame *FrameCombiner::allocate_rescaled_frame() const {
  // STEP 1: Allocate a new AVFrame for the rescaled frame
  AVFrame *rescaled_frame = av_frame_alloc();

  // Check if the allocation was successful
  if (!rescaled_frame) {
    // STEP 2: Failed to allocate the rescaled frame
    std::cerr << "Failed to allocate the rescaled frame." << std::endl;
    return nullptr;
  }

  // STEP 3: Set the format, width, and height of the rescaled frame
  rescaled_frame->format = codec_context_->pix_fmt;
  rescaled_frame->width = codec_context_->width;
  rescaled_frame->height = codec_context_->height;

  // STEP 4: Allocate the buffer for the rescaled frame
  if (av_frame_get_buffer(rescaled_frame, 32) < 0) {
    // STEP 5: Failed to allocate the rescaled frame buffer

    std::cerr << "Failed to allocate the rescaled frame buffer." << std::endl;
    av_frame_free(&rescaled_frame);

    return nullptr;
  }

  return rescaled_frame;
}

bool FrameCombiner::init_rescaled_frame(AVFrame *frame) const {
  // STEP 1: Initialize the image converter (SWSContext)
  SwsContext *swsContext = sws_getContext(
      frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
      codec_context_->width, codec_context_->height, codec_context_->pix_fmt,
      SWS_BICUBIC, nullptr, nullptr, nullptr);

  // Check if the initialization was successful
  if (!swsContext) {

    // Failed to initialize the image converter
    std::cerr << "Failed to initialize the image converter." << std::endl;
    return false;
  }

  // STEP 2: Free the image converter context
  sws_freeContext(swsContext);

  return true;
}

bool FrameCombiner::scale_frame(const AVFrame *src_frame,
                                AVFrame *dst_frame) const {
  // STEP 1: Initialize the image converter (SWSContext)
  SwsContext *swsContext = sws_getContext(
      src_frame->width, src_frame->height,
      static_cast<AVPixelFormat>(src_frame->format), codec_context_->width,
      codec_context_->height, codec_context_->pix_fmt, SWS_BICUBIC, nullptr,
      nullptr, nullptr);

  // Check if the initialization was successful
  if (!swsContext) {
    // Failed to initialize the image converter
    std::cerr << "Failed to initialize the image converter." << std::endl;
    return false;
  }

  // STEP 2: Scale the frame using the image converter
  sws_scale(swsContext, src_frame->data, src_frame->linesize, 0,
            src_frame->height, dst_frame->data, dst_frame->linesize);

  // STEP 3: Free the image converter context
  sws_freeContext(swsContext);

  return true;
}

AVFrame *FrameCombiner::convert_png_to_av_frame(const std::string &file_path) {
  // STEP 1: Open the input file
  AVFormatContext *format_context = nullptr;
  if (!open_input_file(file_path, &format_context)) {
    return nullptr;
  }

  // STEP 2: Retrieve stream information
  if (!retrieve_stream_info(format_context)) {
    avformat_close_input(&format_context);
    return nullptr;
  }

  // STEP 3: Find the video stream
  int video_stream_index = find_video_stream(format_context);
  if (video_stream_index < 0) {
    avformat_close_input(&format_context);
    return nullptr;
  }

  // STEP 4: Get codec parameters
  AVCodecParameters *codec_parameters =
      format_context->streams[video_stream_index]->codecpar;

  // STEP 5: Find the decoder codec
  const AVCodec *codec = find_decoder(codec_parameters->codec_id);
  if (!codec) {
    avformat_close_input(&format_context);
    return nullptr;
  }

  // STEP 6: Create and initialize codec context
  AVCodecContext *codec_context = create_codec_context(codec, codec_parameters);
  if (!codec_context) {
    avformat_close_input(&format_context);
    return nullptr;
  }

  // STEP 7: Create an AVFrame to store the decoded frame
  AVFrame *frame = create_frame();
  if (!frame) {
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
    return nullptr;
  }

  // STEP 8: Decode frames until the end of the file
  if (!decode_frames(format_context, codec_context, frame,
                     video_stream_index)) {
    avcodec_free_context(&codec_context);
    avformat_close_input(&format_context);
    av_frame_free(&frame);
    return nullptr;
  }

  // STEP 9: Cleanup
  avcodec_free_context(&codec_context);
  avformat_close_input(&format_context);

  return frame;
}

bool FrameCombiner::open_input_file(const std::string &file_path,
                                    AVFormatContext **format_context) {
  if (avformat_open_input(format_context, file_path.c_str(), nullptr,
                          nullptr) != 0) {
    std::cerr << "Failed to open input file." << std::endl;
    return false;
  }
  return true;
}

bool FrameCombiner::retrieve_stream_info(AVFormatContext *format_context) {
  if (avformat_find_stream_info(format_context, nullptr) < 0) {
    std::cerr << "Failed to retrieve stream information." << std::endl;
    avformat_close_input(&format_context);
    return false;
  }
  return true;
}

int FrameCombiner::find_video_stream(AVFormatContext *format_context) {
  int video_stream_index = av_find_best_stream(
      format_context, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (video_stream_index < 0) {
    std::cerr << "Failed to find video stream." << std::endl;
    avformat_close_input(&format_context);
  }
  return video_stream_index;
}

const AVCodec *FrameCombiner::find_decoder(AVCodecID codec_id) {
  const AVCodec *codec = avcodec_find_decoder(codec_id);
  if (!codec) {
    std::cerr << "Failed to find decoder." << std::endl;
  }
  return codec;
}

AVCodecContext *
FrameCombiner::create_codec_context(const AVCodec *codec,
                                    AVCodecParameters *codec_parameters) {
  // STEP 1: Create and initialize codec context
  AVCodecContext *codec_context = avcodec_alloc_context3(codec);
  if (!codec_context) {
    std::cerr << "Failed to allocate codec context." << std::endl;
    return nullptr;
  }

  if (avcodec_parameters_to_context(codec_context, codec_parameters) < 0) {
    std::cerr << "Failed to copy codec parameters to context." << std::endl;

    avcodec_free_context(&codec_context);
    return nullptr;
  }

  if (avcodec_open2(codec_context, codec, nullptr) < 0) {
    std::cerr << "Failed to open codec." << std::endl;
    avcodec_free_context(&codec_context);
    return nullptr;
  }

  return codec_context;
}

AVFrame *FrameCombiner::create_frame() {
  AVFrame *frame = av_frame_alloc();
  if (!frame) {
    std::cerr << "Failed to allocate frame." << std::endl;
    return nullptr;
  }

  return frame;
}
bool FrameCombiner::decode_frames(AVFormatContext *format_context,
                                  AVCodecContext *codec_context, AVFrame *frame,
                                  int video_stream_index) {
  AVPacket packet;
  int frame_finished = 0;

  // STEP 1: Read packets from the format context
  while (av_read_frame(format_context, &packet) >= 0) {
    if (packet.stream_index == video_stream_index) {
      // STEP 2: Send packet to the decoder
      if (avcodec_send_packet(codec_context, &packet) < 0) {
        std::cerr << "Failed to send packet to the decoder." << std::endl;
        break;
      }

      // STEP 3: Receive decoded frame from the decoder
      if (avcodec_receive_frame(codec_context, frame) == 0) {
        frame_finished = 1;
        break;
      }
    }
    av_packet_unref(&packet);
  }

  av_packet_unref(&packet);

  // STEP 4: Check if decoding of frame was successful
  if (!frame_finished) {
    std::cerr << "Failed to decode frame." << std::endl;
    return false;
  }

  return true;
}
