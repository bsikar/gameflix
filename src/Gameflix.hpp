#ifndef GAMEFLIX_HPP
#define GAMEFLIX_HPP

#include <string>
#include <variant>
// FFmpeg
extern "C" {
#include <libavformat/avformat.h>
}

class Gameflix {
public:
  Gameflix(std::string input_file_name)
      : Gameflix(input_file_name, "warning") {}

  Gameflix(std::string input_file_name, std::string av_log_level)
      : input_file_name(input_file_name), av_log_level(av_log_level),
        input_format_context(nullptr), video_codec(nullptr),
        video_stream(nullptr), video_decoder_context(nullptr),
        init_input_format_context(init_input_format_context_()),
        init_video_codec(init_video_codec_()),
        init_video_stream(init_video_stream_()),
        init_video_decoder_context(init_video_decoder_context_()) {
    if (std::holds_alternative<std::string>(init_input_format_context)) {
      throw std::runtime_error(
          std::get<std::string>(init_input_format_context));
    }
    if (std::holds_alternative<std::string>(init_video_codec)) {
      throw std::runtime_error(std::get<std::string>(init_video_codec));
    }
    if (std::holds_alternative<std::string>(init_video_stream)) {
      throw std::runtime_error(std::get<std::string>(init_video_stream));
    }
    if (std::holds_alternative<std::string>(init_video_decoder_context)) {
      throw std::runtime_error(
          std::get<std::string>(init_video_decoder_context));
    }
  }

  ~Gameflix();

  void print_video_stream_info();
  void save_video_to_frames();

private:
  std::string input_file_name;
  std::string av_log_level;
  AVFormatContext *input_format_context;
  AVCodec *video_codec;
  AVStream *video_stream;
  AVCodecContext *video_decoder_context;

  std::variant<AVFormatContext *, std::string> init_input_format_context_();
  std::variant<AVCodec *, std::string> init_video_codec_();
  std::variant<AVStream *, std::string> init_video_stream_();
  std::variant<AVCodecContext *, std::string> init_video_decoder_context_();
};

#endif
