#ifndef FRAME_EXTRACTOR
#define FRAME_EXTRACTOR

#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

namespace frame {
/**
 * @brief Structure for extracting frames from a video.
 */
class Extractor {
public:
  /**
   * @brief Constructs a FrameExtractor object.
   * @param video_path The path to the video file.
   */
  Extractor(const std::string &video_path);

  /**
   * @brief Destructor for FrameExtractor.
   */
  ~Extractor();

  /**
   * @brief Extracts frames from the video and saves them as images.
   * @param output_dir The directory to save the extracted frames.
   * @param width The width of the output frames (aspect ratio is maintained).
   */
  void extract_frames(const std::string &output_dir, int width);

  /**
   * @brief Gets the number of leading zeros in the frame count.
   * @return The number of leading zeros.
   */
  int get_leading_zeros();

private:
  AVFormatContext *format_context; /**< The format context for the video. */
  AVCodecContext *codec_context; /**< The codec context for decoding frames. */
  AVCodec *codec;                /**< The codec used for decoding frames. */
  int video_stream_index;        /**< The index of the video stream. */
  int frame_count;               /**< The number of frames extracted. */
  /**
   * @brief Finds the video stream in the format context.
   */
  void find_video_stream();

  /**
   * @brief Initializes the video codec.
   */
  void init_video_codec();

  /**
   * @brief Saves a frame as an image.
   * @param frame The frame to save.
   * @param frame_path The path to save the frame as an image.
   */
  void save_frame_as_image(AVFrame *frame, const std::string &frame_path);

  /**
   * @brief Initializes the PNG codec context.
   * @param png_codec The PNG codec.
   * @param frame The frame for which to initialize the codec context.
   * @return The initialized PNG codec context.
   */
  AVCodecContext *initialize_png_codec_context(AVCodec *png_codec,
                                               AVFrame *frame);

  /**
   * @brief Creates a PNG frame for encoding.
   * @param png_codec_context The PNG codec context.
   * @return The created PNG frame.
   */
  AVFrame *create_png_frame(AVCodecContext *png_codec_context);

  /**
   * @brief Converts a frame to PNG format.
   * @param frame The input frame to convert.
   * @param png_frame The PNG frame to store the converted frame.
   * @param png_codec_context The PNG codec context.
   */
  void convert_frame_to_png(AVFrame *frame, AVFrame *png_frame,
                            AVCodecContext *png_codec_context);

  /**
   * @brief Encodes a PNG frame and writes it to an output file.
   * @param png_codec_context The PNG codec context.
   * @param png_frame The PNG frame to encode.
   * @param output_file The output file stream to write the encoded frame.
   */
  void encode_png_frame(AVCodecContext *png_codec_context, AVFrame *png_frame,
                        std::ofstream &output_file);
};
} // namespace frame
#endif
