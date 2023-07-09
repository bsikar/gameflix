#ifndef FRAME_COMBINER
#define FRAME_COMBINER

#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace frame {
/**
 * @brief Struct for combining frames into a video.
 */
class Combiner {
public:
  /**
   * @brief Constructs a FrameCombiner object.
   * @param output_dir The directory where the output video will be saved.
   */
  Combiner(const std::string &output_dir);

  /**
   * @brief Destroys the FrameCombiner object and cleans up resources.
   */
  ~Combiner();

  /**
   * @brief Combines the frames into a video file.
   * @param output_filename The filename of the output video.
   */
  void combine_frames_to_video(const std::string &output_filename);

private:
  std::string png_dir;           /**< The directory containing PNG frames. */
  std::vector<AVFrame *> frames; /**< The vector of frames. */
  std::vector<std::string> png_files; /**< The vector of PNG file paths. */
  AVFormatContext
      *format_context_; /**< The format context for the output video. */
  AVCodecContext
      *codec_context_; /**< The codec context for encoding the video. */
  AVStream *stream_;   /**< The video stream. */
  AVFrame *frame_;     /**< The current frame being processed. */

  /**
   * @brief Gets the PNG files in the specified directory.
   */
  void get_png_files_in_dir();

  /**
   * @brief Converts PNG frames to AVFrames.
   */
  void convert_pngs_to_frames();

  /**
   * @brief Sets up the video codec for encoding.
   */
  void setup_video_codec();

  /**
   * @brief Opens the output file for writing.
   * @param output_filename The filename of the output video.
   */
  void open_output_file(const std::string &output_filename);

  /**
   * @brief Processes the frames and writes them to the output file.
   */
  void process_frames();

  /**
   * @brief Writes the trailer of the output video file.
   */
  void write_trailer();

  /**
   * @brief Cleans up allocated resources and closes the output file.
   */
  void cleanup_resources();

  /**
   * @brief Converts a PNG file to an AVFrame.
   * @param file_path The path of the PNG file.
   * @return The converted AVFrame.
   */
  AVFrame *convert_png_to_av_frame(const std::string &file_path);

  /**
   * @brief Sets up a new AVFrame.
   * @return The allocated AVFrame.
   */
  AVFrame *setup_frame();

  /**
   * @brief Sets the current frame being processed.
   * @param frame The frame to set as the current frame.
   * @param currentFrame The current frame being processed.
   */
  void set_current_frame(AVFrame *frame, AVFrame *currentFrame);

  /**
   * @brief Encodes and writes a frame to the output video file.
   * @param frame The frame to encode and write.
   * @return `true` if encoding and writing was successful, `false` otherwise.
   */
  bool encode_and_write_frame(AVFrame *frame);

  /**
   * @brief Converts the pixel format of a frame if necessary.
   * @param frame The frame to convert the pixel format.
   * @return The converted AVFrame.
   */
  AVFrame *convert_pixel_format(AVFrame *frame);

  /**
   * @brief Rescales a frame if necessary.
   * @param frame The frame to rescale.
   * @return The rescaled AVFrame.
   */
  AVFrame *rescale_frame_if_necessary(AVFrame *frame);

  /**
   * @brief Checks if the frame size matches the desired size.
   * @param frame The frame to check the size.
   * @return `true` if the frame size matches the desired size, `false`
   * otherwise.
   */
  bool is_frame_size_matching(const AVFrame *frame) const;

  /**
   * @brief Allocates memory for a rescaled frame.
   * @return The allocated rescaled AVFrame.
   */
  AVFrame *allocate_rescaled_frame() const;

  /**
   * @brief Initializes a rescaled frame.
   * @param rescaled_frame The rescaled frame to initialize.
   * @return `true` if initialization was successful, `false` otherwise.
   */
  bool init_rescaled_frame(AVFrame *rescaled_frame) const;

  /**
   * @brief Scales a source frame to a destination frame.
   * @param src_frame The source frame to scale.
   * @param dst_frame The destination frame to store the scaled frame.
   * @return `true` if scaling was successful, `false` otherwise.
   */
  bool scale_frame(const AVFrame *src_frame, AVFrame *dst_frame) const;

  /**
   * @brief Decodes frames from the input video.
   * @param format_context The format context of the input video.
   * @param codec_context The codec context for decoding the frames.
   * @param frame The frame to store the decoded frames.
   * @param video_stream_index The index of the video stream.
   * @return `true` if decoding was successful, `false` otherwise.
   */
  bool decode_frames(AVFormatContext *format_context,
                     AVCodecContext *codec_context, AVFrame *frame,
                     int video_stream_index);

  /**
   * @brief Creates a new AVFrame.
   * @return The created AVFrame.
   */
  AVFrame *create_frame();

  /**
   * @brief Creates a new AVCodecContext.
   * @param codec The AVCodec for the codec context.
   * @param codec_parameters The codec parameters for the codec context.
   * @return The created AVCodecContext.
   */
  AVCodecContext *create_codec_context(const AVCodec *codec,
                                       AVCodecParameters *codec_parameters);

  /**
   * @brief Finds a decoder for the specified codec ID.
   * @param codec_id The codec ID.
   * @return The found AVCodec.
   */
  const AVCodec *find_decoder(AVCodecID codec_id);

  /**
   * @brief Finds the index of the video stream in the format context.
   * @param format_context The format context.
   * @return The index of the video stream if found, -1 otherwise.
   */
  int find_video_stream(AVFormatContext *format_context);

  /**
   * @brief Retrieves the stream information from the format context.
   * @param format_context The format context.
   * @return `true` if retrieval was successful, `false` otherwise.
   */
  bool retrieve_stream_info(AVFormatContext *format_context);

  /**
   * @brief Opens the input file for reading.
   * @param file_path The path of the input file.
   * @param format_context The pointer to the format context to store the opened
   * file.
   * @return `true` if opening was successful, `false` otherwise.
   */
  bool open_input_file(const std::string &file_path,
                       AVFormatContext **format_context);
};
} // namespace frame
#endif
