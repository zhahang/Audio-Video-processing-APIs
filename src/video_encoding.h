#ifndef SAURON_UTIL_VIDEO_ENCODING_H_
#define SAURON_UTIL_VIDEO_ENCODING_H_

#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;
using namespace cv;

#pragma warning(disable: 4996)

bool videoEncoding(vector<string>& imgs_names, cv::Size output_size, int output_br, const char * output_file);

bool convertFormat(const char* input_h264, const char* output_video);

bool Image2Video(const char* input_path, const char *image_fmt, int start_id, int image_num,
	const char* output_path, const char* output_name, Size output_size, int output_br);
// namespace sauron
#endif

