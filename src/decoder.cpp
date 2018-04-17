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
        mSkipFrameCount = 0;
        mVideoStreamIdx = -1;
        m_out_buffer = NULL;
        mIsInit = false;
    }
    
    Decoder::~Decoder(){
        if(mSwsCtx != NULL){
           sws_freeContext(mSwsCtx);
            mSwsCtx = NULL;
        }
        if(mPacket != NULL){
            av_packet_unref(mPacket);
            av_free(mPacket);
            mPacket = NULL;
        }
        if(mFrameYUV != NULL){
            av_frame_free(&mFrameYUV);
            mFrameYUV = NULL;
        }
        if(mFrame != NULL){
            av_frame_free(&mFrame);
            mFrame = NULL;
        }
        if(mCodecCtx != NULL){
            avcodec_close(mCodecCtx);
            mCodecCtx = NULL;
        }
        if(mFmtCtx != NULL){
            avformat_close_input(&mFmtCtx);
            avformat_free_context(mFmtCtx);
        }
        if(m_out_buffer != NULL){
            av_free(m_out_buffer);
            m_out_buffer = NULL;
        }
        
#ifdef _ZH_DEBUG
        fclose(fp_yuv);
#endif
    }
    
    int Decoder::init(const char *filePath){
        if(mIsInit)
            return -1;
        
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
        mConfig.width = mCodecCtx->width;
        mConfig.height = mCodecCtx->height;
        mConfig.bitRate = mCodecCtx->bit_rate;
        mConfig.frameRate = mFmtCtx->streams[mVideoStreamIdx]->r_frame_rate;
        mConfig.duration = mFmtCtx->streams[mVideoStreamIdx]->nb_frames;
        
        mCodec = avcodec_find_decoder(mCodecCtx->codec_id);
        if(mCodec == NULL){
            printf("Codec not found.\n");
            return -1;
        }
        if(avcodec_open2(mCodecCtx, mCodec, NULL) < 0){
            printf("Could not open codec.\n");
            return -1;
        }
        
//        mFrame = av_frame_alloc();
        mFrameYUV = av_frame_alloc();
        mPacket=(AVPacket *)av_malloc(sizeof(AVPacket));
        
        m_out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P,
                            mCodecCtx->width, mCodecCtx->height,1));
        av_image_fill_arrays(mFrameYUV->data, mFrameYUV->linesize, m_out_buffer,
                            AV_PIX_FMT_YUV420P, mCodecCtx->width, mCodecCtx->height, 1);
        mSwsCtx = sws_getContext(mCodecCtx->width, mCodecCtx->height, mCodecCtx->pix_fmt,
                            mCodecCtx->width, mCodecCtx->height, AV_PIX_FMT_YUV420P,
                            SWS_BICUBIC, NULL, NULL, NULL);
        mIsInit = true;
        return 0;
    }
    
    int Decoder::decodeOneFrame(unsigned char **yuv_buffer){
        if(!mIsInit){
            printf("error; please init decoder first.\n");
            return -1;
        }
        
        AVFrame *frame = NULL;
        int ret = decodeOneFrame(frame);
        if(ret < 0){
            printf("error: decode frame failed.\n");
            return -1;
        }
        sws_scale(mSwsCtx, (const unsigned char* const*)frame->data, frame->linesize,
                  0, mCodecCtx->height, mFrameYUV->data, mFrameYUV->linesize);
        *yuv_buffer = m_out_buffer;
        return -1;
    }
    
    int Decoder::decodeOneFrame(AVFrame *frame){
        if(!mIsInit){
            printf("error; please init decoder first.\n");
            return -1;
        }
        
        int ret = -1;
        int got_picture = -1;
        while(true){
            ret = av_read_frame(mFmtCtx, mPacket);
            if(ret < 0)
                break;
            if(mPacket->stream_index == mVideoStreamIdx){
                ret = avcodec_decode_video2(mCodecCtx, mFrameYUV, &got_picture, mPacket);
                if(ret < 0){
                    printf("Decode Error:ret=%d\n", ret);
                    av_packet_unref(mPacket);
                    return -1;
                }
                if(got_picture > 0){
                    *frame = *mFrameYUV;
                    mCurrFrameIdx++;
                    av_packet_unref(mPacket);
#ifdef _ZH_DEBUG
                    printf("decode frame %d\n", mCurrFrameIdx);
#endif
                    return 0;
                }
                mSkipFrameCount++;
                av_packet_unref(mPacket);
            }
            av_packet_unref(mPacket);
        }
        av_packet_unref(mPacket);

        while(mSkipFrameCount > 0){
            ret = avcodec_decode_video2(mCodecCtx, mFrameYUV, &got_picture, NULL);
            if(ret < 0)
                return -1;
            if(got_picture <= 0){
                *frame = *mFrameYUV;
                mCurrFrameIdx++;
                mSkipFrameCount--;
#ifdef _ZH_DEBUG
                printf("decode frame %d\n", mCurrFrameIdx);
#endif
                return 0;
            }
            else
                return -1;
        }
        return -1;
    }
    
    int Decoder::seekToBegin(){
        if(!mIsInit){
            printf("error; please init decoder first.\n");
            return -1;
        }
        
        int ret = av_seek_frame(mFmtCtx, -1, 0, AVSEEK_FLAG_BACKWARD);
        if(ret >= 0)
            mCurrFrameIdx = 0;
        return ret;
    }
    
    int Decoder::getDecoderConfig(ZH::DecoderConfig *config){
        if(!mIsInit){
            printf("error; please init decoder first.\n");
            return -1;
        }
        if(config == NULL)
            return -1;
        *config = mConfig;
        return 0;
    }
};
