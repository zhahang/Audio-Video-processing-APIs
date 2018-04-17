//
//  encoder.hpp
//  FFmpegDecoder
//
//  Created by zhahang on 2018/4/9.
//  Copyright © 2018年 zhahang. All rights reserved.
//

#ifndef _ZH_ENCODER_H
#define _ZH_ENCODER_H

#include <stdio.h>
#include <string>

#ifdef __cplusplus
extern "C"
{
#endif
#include "libavcodec/avcodec.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#ifdef __cplusplus
};
#endif

namespace ZH {
    
    struct EncoderConfig{
        int width;
        int height;
        int64_t bitRate;
        AVRational frameRate;
        EncoderConfig(int _width, int _height, AVRational _frameRate, int64_t _bitRate = 2000000){
            width = _width;
            height = _height;
            bitRate = _bitRate;
            frameRate = _frameRate;
        }
        EncoderConfig(){
            
        }
    };
    
    class Encoder{
    public:
        Encoder();
        ~Encoder();
        int init(const char *path, struct EncoderConfig *config);
        int encodeOneFrame(uint8_t *buffer, int width, int height, int but_format);
        int encodeOneFrame(AVFrame *frame_yuv);
        int formatFrame(AVFrame *src_frame, AVFrame *dst_frame);
        int encodeFinish();
    private:
        bool convertFormat(const char* input_h264, const char* output_video);
        
    private:
        EncoderConfig mConfig;
        AVFormatContext *mFmtCtx;
        AVCodecContext *mCodecCtx;
        AVStream *mStreamE;
        AVCodec *mCodec;
        SwsContext *mSwsCtx;
        AVPacket *mPacket;
        AVFrame *mFrameYUV;
        AVFrame *mFrame;
        
        int mCurrFrameIdx;
        bool mIsInit;
        std::string mH264FilePath;
        std::string mVideoFilePath;
    };
}

#endif /* encoder_hpp */
