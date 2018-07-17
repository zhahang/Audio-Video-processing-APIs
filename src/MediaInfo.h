//
//  TDMediaInfo.hpp
//  FFmpegDecoder
//
//  Created by zhahang on 2018/7/16.
//  Copyright © 2018年 zhahang. All rights reserved.
//

#ifndef _TD_MEDIA_INFO_H
#define _TD_MEDIA_INFO_H

#include <stdio.h>
#include <string>

struct TDMediaInfo {
    std::string filePath;
    std::string fileName;
    std::string fileSuffix;
    int vWidth;
    int vHeight;
    int64_t vBitRate; // b/s
    int64_t vTotalFrames;
    float vDuration; // second
    float vFrameRate;
    float vRotateAngle;
    bool vHasBFrame;
    std::string vCodecName;
    std::string vPixelFmt;
    int aSampleRate;
    int aChannels;
    int64_t aTotalFrames;
    int64_t aBitRate;
    int64_t aMaxBitRate;
    float aDuration;
    std::string aCodecName;
    std::string aSampleFmt;
    bool isCheckCodec;
    TDMediaInfo(std::string &_path){
        filePath = _path;
        isCheckCodec = true;
    }
};

int getMediaInfo(TDMediaInfo *mediaInfo);

#endif /* _TD_MEDIA_INFO_H */
