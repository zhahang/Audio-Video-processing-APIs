//
//  decoder.cpp
//  FFmpegDecoder
//
//  Created by zhahang on 2017/12/14.
//  Copyright © 2017年 zhahang. All rights reserved.
//


#include "decoder.h"


namespace ZH {
    Decoder::Decoder(){
        mFmtCtx = NULL;
        mCodecCtx = NULL;
        mCodec = NULL;
        mSwsCtx = NULL;
        mPacket = NULL;
        mFrameYUV = NULL;
        mFrame = NULL;
        
        mCurrFrameIdx = 0;
        mVideoStreamIdx = -1;
        m_out_buffer = NULL;
    }
    
    Decoder::~Decoder(){
        if(mSwsCtx != NULL)
            sws_freeContext(mSwsCtx);
        if(mFrameYUV != NULL)
            av_frame_free(&mFrameYUV);
        if(mFrame != NULL)
            av_frame_free(&mFrame);
        if(mCodecCtx != NULL)
            avcodec_close(mCodecCtx);
        if(mFmtCtx != NULL)
            avformat_close_input(&mFmtCtx);
        
#ifdef _ZH_DEBUG
        fclose(fp_yuv);
#endif
    }
    
    int Decoder::init(const char *filePath){
        av_register_all();
        mFmtCtx = avformat_alloc_context();
        if(avformat_open_input(&mFmtCtx, filePath, NULL, NULL) != 0){
            printf("Couldn't open input stream, filePath=%s.\n", filePath);
            return -1;
        }
        if(avformat_find_stream_info(mFmtCtx, NULL) < 0){
            printf("Couldn't find stream information.\n");
            return -1;
        }
        for(int i=0; i < mFmtCtx->nb_streams; i++)
            if(mFmtCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
                mVideoStreamIdx=i;
                break;
            }
        
        if(mVideoStreamIdx == -1){
            printf("Didn't find a video stream.\n");
            return -1;
        }
        
        mCodecCtx = mFmtCtx->streams[mVideoStreamIdx]->codec;
        mCodec = avcodec_find_decoder(mCodecCtx->codec_id);
        if(mCodec == NULL){
            printf("Codec not found.\n");
            return -1;
        }
        if(avcodec_open2(mCodecCtx, mCodec, NULL) < 0){
            printf("Could not open codec.\n");
            return -1;
        }
        
        mFrame = av_frame_alloc();
        mFrameYUV = av_frame_alloc();
        mPacket=(AVPacket *)av_malloc(sizeof(AVPacket));
        
        m_out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                            mCodecCtx->width, mCodecCtx->height,1));
        av_image_fill_arrays(mFrameYUV->data, mFrameYUV->linesize, m_out_buffer,
                            AV_PIX_FMT_YUV420P, mCodecCtx->width, mCodecCtx->height, 1);
        mSwsCtx = sws_getContext(mCodecCtx->width, mCodecCtx->height, mCodecCtx->pix_fmt,
                            mCodecCtx->width, mCodecCtx->height, AV_PIX_FMT_YUV420P,
                            SWS_BICUBIC, NULL, NULL, NULL);
        return 0;
    }
    
    int Decoder::decodeOneFrame(unsigned char **yuv_buffer){
        int ret, got_picture;
        while(av_read_frame(mFmtCtx, mPacket)>=0){
            if(mPacket->stream_index==mVideoStreamIdx){
                ret = avcodec_decode_video2(mCodecCtx, mFrame, &got_picture, mPacket);
                if(ret < 0){
                    printf("Decode Error.\n");
                    av_free_packet(mPacket);
                    return -1;
                }
                if(got_picture){
                    sws_scale(mSwsCtx, (const unsigned char* const*)mFrame->data, mFrame->linesize,
                              0, mCodecCtx->height, mFrameYUV->data, mFrameYUV->linesize);
                    *yuv_buffer = m_out_buffer;
                    mCurrFrameIdx++;
                    return 0;
                }
            }
            av_free_packet(mPacket);
        }
        
        //flush decoder
        //FIX: Flush Frames remained in Codec
        while (true) {
            ret = avcodec_decode_video2(mCodecCtx, mFrame, &got_picture, mPacket);
            if (ret < 0)
                break;
            if (!got_picture)
                break;
            sws_scale(mSwsCtx, (const unsigned char* const*)mFrame->data, mFrame->linesize,
                      0, mCodecCtx->height, mFrameYUV->data, mFrameYUV->linesize);
            *yuv_buffer = m_out_buffer;
            mCurrFrameIdx++;
            return 0;
        }
        return -1;
    }
    
    int Decoder::seekToBegin(){
        int ret = av_seek_frame(mFmtCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
        if(ret >= 0)
            mCurrFrameIdx = 0;
        return ret;
    }
};
