#ifndef _ZH_VIDEO_FILE_CUT_H
#define _ZH_VIDEO_FILE_CUT_H

#include <string>


bool VideoFileCut(std::string& input_filename, std::string& output_filename, int start_frame, int end_frame, int *offset);

#endif