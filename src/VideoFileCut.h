#ifndef MERGE_AUDIO_INTO_VIDEO_H_
#define MERGE_AUDIO_INTO_VIDEO_H_

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavformat/avformat.h>
#include <libavutil/mathematics.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/avfiltergraph.h>
#include <libswresample/swresample.h>  

#ifdef __cplusplus
}
#endif

using namespace std;

bool VideoFileCut(std::string& input_filename, std::string& output_filename, int start_frame, int end_frame, int *offset)

#endif