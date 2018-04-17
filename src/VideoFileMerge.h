#ifndef VIDEO_FILE_MERGE_H_
#define VIDEO_FILE_MERGE_H_

#include <string>
#include <vector>



bool VideoFileMerge(std::vector<std::string> &input_filename_list, std::vector<int> &offsets, std::string &output_filename);

bool VideoFileMergeDefault(std::vector<std::string> &input_filename_list, std::string &output_filename);


#endif
