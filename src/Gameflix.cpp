#include "Gameflix.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
// FFmpeg
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/pixdesc.h>
#include <libswscale/swscale.h>
}
