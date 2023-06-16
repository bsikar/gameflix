#ifndef FRAME_EXTRACTOR
#define FRAME_EXTRACTOR

#include <string>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

/**
 * FrameExtractor struct for extracting frames from a video file.
 *
 * The FrameExtractor struct allows extracting frames from a video file using
 * the FFmpeg library.
 * It provides functionalities to open a video file, retrieve stream
 * information, find the video stream,
 *
 * initialize the video codec, and extract frames from the video stream.
 */
struct FrameExtractor {
  AVFormatContext
      *format_context; /**< Pointer to the AVFormatContext struct representing
                          the format context of the video. */
  AVCodecContext
      *codec_context; /**< Pointer to the AVCodecContext struct representing the
                         codec context for decoding frames. */
  AVCodec *
      codec; /**< Pointer to the AVCodec struct representing the video codec. */
  int video_stream_index; /**< Index of the video stream in the format context.
                           */
  int frame_count;        /**< Number of frames extracted. */

  /**
   * Constructor that initializes the FrameExtractor object.
   *
   * This constructor takes a video_path as input and initializes the
   * FrameExtractor object. It performs the following steps:
   * 1. Opens the video file specified by the video_path.
   * 2. Retrieves the stream information from the video file.
   * 3. Finds the video stream in the format context.
   * 4. Initializes the video codec for decoding frames.
   *
   * @param video_path The path to the video file to be processed.
   * @throws std::runtime_error if any of the initialization steps fail.
   */
  FrameExtractor(const std::string &video_path);

  /**
   * Destructor to clean up allocated resources.
   */
  ~FrameExtractor();

  /**
   * Extracts frames from the video and saves them as images in the output
   * directory.
   *
   * This function extracts frames from the video stream and saves
   * them as images in the specified output directory.
   * It performs the following steps:
   * 1. Reads packets from the format
   * context until the end of the video stream is reached.
   * 2. Sends packets to the codec context for decoding and receives frames.
   * 3. Saves each received frame as an image file in the output directory.
   *
   * @param output_dir The directory where the extracted frames will be saved.
   */
  void extract_frames(const std::string &output_dir, int width);

  int get_leading_zeros() {
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();

    // Calculate the total number of frames
    int total_frames = 0;
    while (av_read_frame(format_context, &packet) >= 0) {
      if (packet.stream_index == video_stream_index) {
        total_frames += 1;
      }
      av_packet_unref(&packet);
    }
    av_seek_frame(format_context, video_stream_index, 0, AVSEEK_FLAG_BACKWARD);

    // Determine the width for leading zeros
    int width = 1;
    int temp = total_frames;
    while (temp /= 10) {
      width += 1;
    }
    av_frame_free(&frame);
    return width;
  }

private:
  /**
   * Finds the video stream in the format context.
   *
   * This function iterates over the streams in the format context
   * and finds the video stream based on its codec type.
   * It sets the video_stream_index accordingly.
   *
   * @throws std::runtime_error if the video stream is not found.
   */
  void find_video_stream();

  /**
   * Initializes the video codec for decoding frames.
   *
   * This function allocates the codec context and sets its parameters based on
   * the video stream in the format context. It also finds the
   * video decoder and opens the codec for decoding.
   *
   * @throws std::runtime_error if any of the initialization steps fail.
   */
  void init_video_codec();

  /**
   * Saves a frame as an image file.
   *
   * This function takes an AVFrame representing a video frame and saves it as
   * an image file in PNG format at the specified frame_path. It performs the
   * following steps:
   * 1. Finds the PNG codec and initializes the codec context.
   * 2. Converts the input frame to the PNG pixel format.
   * 3. Writes the PNG data to the output file.
   *
   * @param frame The AVFrame representing the video frame to be saved.
   * @param frame_path The path to the output image file.
   * @throws std::runtime_error if any of the steps fail.
   */
  void save_frame_as_image(AVFrame *frame, const std::string &frame_path);
};

#endif
