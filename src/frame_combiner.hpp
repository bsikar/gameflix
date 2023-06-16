#ifndef FRAME_COMBINER
#define FRAME_COMBINER

#include <string>
#include <vector>

static const unsigned int frame_width = 1920;
static const unsigned int frame_height = 1080;

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

/**
 * FrameCombiner struct for combining frames into a video file.
 *
 * The FrameCombiner struct allows combining a collection of frames into a video
 * file. It utilizes the FFmpeg library for handling audio/video formats and
 * encoding the video.
 */
struct FrameCombiner {
  std::string png_dir; /**< A string representing the path to the ouput dir */
  std::vector<AVFrame *> frames; /**< A vector of AVFrame object pointers
                                  representing the frames to be combined. */
  std::vector<std::string> png_files;

  AVFormatContext *format_context_;
  AVCodecContext *codec_context_;
  AVStream *stream_;
  AVFrame *frame_;

  /**
   * Constructor that initializes the FrameCombiner object.
   *
   * This constructor takes the output directory as input and initializes the
   * FrameCombiner object.
   *
   * @param output_dir The directory where the combined video file will be
   * saved.
   */
  FrameCombiner(const std::string &output_dir);

  /**
   * Destructor to clean up allocated resources.
   */
  ~FrameCombiner();

  /**
   * Combines the frames into a video file.
   *
   * This function combines the frames stored in the frames vector
   * into a video file.
   * It performs the following steps:
   *
   * 1. Initializes the output video format context and sets its properties.
   * 2. Initializes the video encoder codec and sets its parameters.
   * 3. Opens the output video file for writing.
   * 4. Allocates the video frame and sets its properties.
   * 5. Encodes each frame and writes the encoded packets to the output file.
   * 6. Writes the video stream trailer and releases the allocated resources.
   *
   * @param output_video The path to the output video file.
   * @throws std::runtime_error if any of the encoding or writing steps fail.
   */
  void combine_frames_to_video(const std::string &output_filename);

  void get_png_files_in_dir();

  void convert_pngs_to_frames();

  void setup_video_codec();

  void open_output_file(const std::string &output_filename);

  void process_frames();

  void write_trailer();

  void cleanup_resources();

  AVFrame *convert_png_to_av_frame(const std::string &file_path);
};

#endif
