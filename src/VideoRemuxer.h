#ifndef _ZH_VIDEO_REMUXER_H_
#define _ZH_VIDEO_REMUXER_H_

#include <string>


#ifndef MAX_AUDIO_FRAME_SIZE
#define MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio  
#endif



bool VideoRemuxerEncode(std::string &video_name, std::string &audio_name,
                        int offset, std::string &output_video_name);

bool VideoRemuxer(std::string &in_filename_v, std::string &in_filename_a,
                  int offset, std::string &out_filename);

#endif