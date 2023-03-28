#ifndef VIDEO_H
#define VIDEO_H

#include <vector>

using namespace std;

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using f32 = float;

struct Frame {
  u32 width;
  u32 height;
  u16 frame_count;
  vector<u8> bytes;

  // default constructor
  Frame() : width(0), height(0), frame_count(0) {}

  // constructor with parameters
  Frame(u32 width, u32 height, u16 frame_count)
      : width(width), height(height), frame_count(frame_count) {}

  // add a byte to the bytes vector
  void addbyte(u8 byte) { bytes.push_back(byte); }

  // get the size of the bytes vector
  size_t getsize() const { return bytes.size(); }
};

struct Video {
  f32 framerate;
  u32 width;
  u32 height;
  vector<Frame> frames;

  // default constructor
  Video() : framerate(0.0f), width(0), height(0) {}

  // constructor with parameters
  Video(f32 framerate, u32 width, u32 height)
      : framerate(framerate), width(width), height(height) {}

  // add a frame to the frames vector
  void addframe(const Frame &frame) { frames.push_back(frame); }

  // get the number of frames in the video
  size_t getframecount() const { return frames.size(); }
};
#endif
