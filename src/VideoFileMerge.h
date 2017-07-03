#ifndef VIDEO_FILE_MERGE_H_
#define VIDEO_FILE_MERGE_H_

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

bool VideoFileMerge(std::vector<std::string> &input_filename_list, std::vector<int> &offsets, std::string &output_filename);


#endif