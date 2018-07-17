//
//  TDMediaInfo.cpp
//  FFmpegDecoder
//
//  Created by zhahang on 2018/7/16.
//  Copyright © 2018年 zhahang. All rights reserved.
//

#include "TDMediaInfo.h"
#include <iostream>
#include <sstream>

#ifdef __cplusplus
extern "C"{
#endif
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
#ifdef __cplusplus
}
#endif

using namespace std;


float getVideoRotate(AVStream *in_stream){
    AVDictionaryEntry *item = NULL;
    item = av_dict_get(in_stream->metadata, "rotate", item, 0);
    float rotate = .0f;
    if(item == NULL){
        rotate = NULL;
        return .0f;
    }
    string value = item->value;
    istringstream iss(value);
    iss >> rotate;
    return rotate;
}

int getMediaInfo(TDMediaInfo *mediaInfo){
    if(!mediaInfo || mediaInfo->filePath.size() < 1){
        cout<<"getMediaInfo() invalid argument"<<endl;
        return -1;
    }
    
    AVFormatContext *fmtCtx = NULL;
    AVCodecParameters *vCodecPar = NULL, *aCodecPar = NULL;
    AVCodecContext *vCodecCtx = NULL, *aCodecCtx = NULL;
    AVCodec *vCodec = NULL, *aCodec = NULL;
//    AVDictionaryEntry *dict = NULL;
    int videoIdx = -1, audioIdx = -1;
    int ret = -1;
    
    av_register_all();
    ret = avformat_open_input(&fmtCtx, mediaInfo->filePath.c_str(), NULL, NULL);
    if(ret != 0){
        av_log(NULL, AV_LOG_ERROR, "Can't open file\n");
        goto _END;
    }
    ret = avformat_find_stream_info(fmtCtx, NULL);
    if(ret < 0){
        av_log(NULL, AV_LOG_ERROR, "Can't get stream info\n");
        goto _END;
    }
    
    videoIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    audioIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(videoIdx < 0 && audioIdx < 0){
        av_log(NULL, AV_LOG_ERROR, "Can't find video and audio stream in input file\n");
        ret = -1;
        goto _END;
    }
    
    if(videoIdx >= 0){
        AVStream *vStream = fmtCtx->streams[videoIdx];
        vCodecPar = vStream->codecpar;
        vCodec = avcodec_find_decoder(vCodecPar->codec_id);
        if(!vCodec){
            av_log(NULL, AV_LOG_ERROR, "Can't find decoder\n");
            ret = -1;
            goto _END;
        }
        vCodecCtx = avcodec_alloc_context3(vCodec);
        if (!vCodecCtx) {
            av_log(NULL, AV_LOG_ERROR, "Can't allocate decoder context\n");
            ret = AVERROR(ENOMEM);
            goto _END;
        }
        ret = avcodec_parameters_to_context(vCodecCtx, vCodecPar);
        if (ret) {
            av_log(NULL, AV_LOG_ERROR, "Can't copy decoder context\n");
            goto _END;
        }
        ret = avcodec_open2(vCodecCtx, vCodec, NULL);
        if (ret < 0) {
            av_log(vCodecCtx, AV_LOG_ERROR, "Can't open decoder\n");
            goto _END;
        }
        
//        mediaInfo->fileName = fmtCtx->filename;
//        mediaInfo->fileSuffix = fmtCtx->iformat->extensions;
        mediaInfo->vWidth = vCodecCtx->width;
        mediaInfo->vHeight = vCodecCtx->height;
        mediaInfo->vBitRate = vCodecCtx->bit_rate;
        mediaInfo->vTotalFrames = vStream->nb_frames;
        mediaInfo->vDuration = vStream->duration * av_q2d(vStream->time_base);
        mediaInfo->vFrameRate = vStream->r_frame_rate.den > 0 ? av_q2d(vStream->r_frame_rate) : av_q2d(vCodecCtx->framerate);
        mediaInfo->vRotateAngle = getVideoRotate(vStream);
        mediaInfo->vHasBFrame = vCodecCtx->has_b_frames;
        mediaInfo->vCodecName = avcodec_get_name(vCodecPar->codec_id);
        mediaInfo->vPixelFmt = to_string(vCodecCtx->pix_fmt);
    }
    if(audioIdx >= 0){
        AVStream *aStream = fmtCtx->streams[audioIdx];
        aCodecPar = aStream->codecpar;
        aCodec = avcodec_find_decoder(aCodecPar->codec_id);
        if(!aCodec){
            av_log(NULL, AV_LOG_ERROR, "Can't find decoder\n");
            ret = -1;
            goto _END;
        }
        aCodecCtx = avcodec_alloc_context3(aCodec);
        if (!aCodecCtx) {
            av_log(NULL, AV_LOG_ERROR, "Can't allocate decoder context\n");
            ret = AVERROR(ENOMEM);
            goto _END;
        }
        ret = avcodec_parameters_to_context(aCodecCtx, aCodecPar);
        if (ret) {
            av_log(NULL, AV_LOG_ERROR, "Can't copy decoder context\n");
            goto _END;
        }
        ret = avcodec_open2(aCodecCtx, aCodec, NULL);
        if (ret < 0) {
            av_log(aCodecCtx, AV_LOG_ERROR, "Can't open decoder\n");
            goto _END;
        }
        mediaInfo->aSampleRate = aCodecCtx->sample_rate;
        mediaInfo->aChannels = aCodecCtx->channels;
        mediaInfo->aTotalFrames = aStream->nb_frames;
        mediaInfo->aBitRate = aCodecCtx->bit_rate;
//        mediaInfo->aMaxBitRate = aCodecCtx->max
        mediaInfo->aDuration = aStream->duration * av_q2d(aStream->time_base);
        mediaInfo->aCodecName = avcodec_get_name(aCodecCtx->codec_id);
        mediaInfo->aSampleFmt = to_string(aCodecCtx->sample_fmt);
    }
    
_END:
    if(vCodecCtx){
        avcodec_close(vCodecCtx);
        avcodec_free_context(&vCodecCtx);
    }
    if(aCodecCtx){
        avcodec_close(aCodecCtx);
        avcodec_free_context(&aCodecCtx);
    }
    if(fmtCtx){
        avformat_close_input(&fmtCtx);
        avformat_free_context(fmtCtx);
    }
    return ret;
}














