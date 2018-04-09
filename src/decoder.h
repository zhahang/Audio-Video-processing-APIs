//
//  decoder.hpp
//  FFmpegDecoder
//
//  Created by zhahang on 2017/12/14.
//  Copyright © 2017年 zhahang. All rights reserved.
//

#ifndef _ZH_DECODER_H
#define _ZH_DECODER_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#ifdef __cplusplus
};
#endif

namespace ZH{
    class Decoder{
    public:
        Decoder();
        ~Decoder();
        
        int init(const char *filePath);
        int decodeOneFrame(unsigned char **yuv_buffer);
        int seekToBegin();
    private:
        AVFormatContext *mFmtCtx;
        AVCodecContext *mCodecCtx;
        AVCodec *mCodec;
        SwsContext *mSwsCtx;
        AVPacket *mPacket;
        AVFrame *mFrameYUV;
        AVFrame *mFrame;
        
        int mCurrFrameIdx;
        int mVideoStreamIdx;
        unsigned char *m_out_buffer;
    };
}

#endif /* decoder_hpp */
