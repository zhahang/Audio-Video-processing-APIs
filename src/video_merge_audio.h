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

#ifndef MAX_AUDIO_FRAME_SIZE
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio  
#endif

using namespace std;

bool VideoRemuxerEncode(string &video_name, string &audio_name, int offset, string &output_video_name);

bool VideoRemuxer(string &in_filename_v, string &in_filename_a, int offset, string &out_filename);

#endif