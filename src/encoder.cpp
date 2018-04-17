//
//  encoder.cpp
//  FFmpegDecoder
//
//  Created by zhahang on 2018/4/9.
//  Copyright © 2018年 zhahang. All rights reserved.
//

#include "encoder.h"

namespace ZH {
    
    Encoder::Encoder():mConfig(), mFmtCtx(NULL), mCodecCtx(NULL), mStreamE(NULL),
                       mCodec(NULL), mSwsCtx(NULL), mPacket(NULL), mFrameYUV(NULL), mFrame(NULL),
                       mCurrFrameIdx(0), mIsInit(false), mH264FilePath(""), mVideoFilePath(""){
    }
    
    Encoder::~Encoder(){
//        if(mSwsCtx != NULL)
//            sws_freeContext(mSwsCtx);
        if(mFrameYUV != NULL){
            av_frame_free(&mFrameYUV);
            mFrameYUV = NULL;
        }
        if(mFrame != NULL){
            av_frame_free(&mFrame);
            mFrame = NULL;
        }
        if(mPacket != NULL){
            av_free(mPacket);
            mPacket = NULL;
        }
        if(mCodecCtx != NULL){
            avcodec_close(mCodecCtx);
            mCodecCtx = NULL;
        }
        if(mFmtCtx != NULL){
            avformat_close_input(&mFmtCtx);
            avformat_free_context(mFmtCtx);
            mFmtCtx = NULL;
        }
        mIsInit = false;
    }
    
    int Encoder::init(const char *path, struct EncoderConfig *config){
        if(mIsInit)
            return -1;
        
        if(path == NULL){
            printf("init encoder failed: path is NULL.\n");
            return -1;
        }
        if(config == NULL){
            printf("init encoder failed: config is NULL.\n");
            return -1;
        }
        mVideoFilePath = path;
        mH264FilePath = mVideoFilePath + ".h264";
        mConfig = *config;
        
        AVOutputFormat   *pOutFmtE;
        
        int nWidth  = mConfig.width;
        int nHeight = mConfig.height;
        int nByte = nWidth * nHeight * 3 ;
        int64_t br = mConfig.bitRate;
        
        av_register_all();
        avformat_alloc_output_context2(&mFmtCtx, NULL, NULL, mH264FilePath.c_str());
        pOutFmtE = mFmtCtx->oformat;
        if (avio_open(&mFmtCtx->pb, mH264FilePath.c_str(), AVIO_FLAG_READ_WRITE) < 0){
            printf("error: cannot OPEN Format Context...\n");
            return -1;
        }
        
        mStreamE = avformat_new_stream(mFmtCtx, 0);
        if (mStreamE == NULL){
            printf("error: cannot Open Stream...\n");
            return -1;
        }
        
        mCodecCtx = mStreamE->codec;
        mCodecCtx->codec_id = pOutFmtE->video_codec;
        mCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
        mCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
        mCodecCtx->width = nWidth;
        mCodecCtx->height = nHeight;
        mCodecCtx->time_base.num = 1;
        mCodecCtx->time_base.den = av_q2d(mConfig.frameRate);
     
        //CBR
        mCodecCtx->bit_rate = br;
        mCodecCtx->rc_min_rate = br;
        mCodecCtx->rc_max_rate = br;
        mCodecCtx->bit_rate_tolerance = (int)br;
        mCodecCtx->rc_buffer_size = (int)br;
        mCodecCtx->rc_initial_buffer_occupancy = mCodecCtx->rc_buffer_size * 3 / 4;
        mCodecCtx->rc_buffer_aggressivity = (float)1.0;
        mCodecCtx->rc_initial_cplx = 0.5;
        
        mCodecCtx->gop_size = 30;
        mCodecCtx->qmin = 10;
        mCodecCtx->qmax = 51;
        mCodecCtx->sample_aspect_ratio.num = 1; // nWidth;
        mCodecCtx->sample_aspect_ratio.den = 1; // nHeight;
        
        mCodec = avcodec_find_encoder(mCodecCtx->codec_id);
        if (!mCodec)
        {
            printf("error: cannot FIND Encoder...\n");
            return -1;
        }
        if (avcodec_open2(mCodecCtx, mCodec, NULL) < 0)
        {
            printf("error: cannot OPEN Encoder");
            return -1;
        }
        
        mPacket=(AVPacket *)av_malloc(sizeof(AVPacket));
//        av_new_packet(mPacket, nByte);
        
        int ret = avformat_write_header(mFmtCtx, NULL);
        if(ret < 0){
            printf("error: avformat_write_header failed!\n");
            return -1;
        }
        
        mFrame = av_frame_alloc();
        mFrameYUV = av_frame_alloc();

        
        mIsInit = true;
        return ret;
    }
    
    int Encoder::formatFrame(AVFrame *src_frame, AVFrame *dst_frame){
        if(src_frame == NULL || dst_frame == NULL){
            printf("param is null.\n");
            return -1;
        }
        
        mSwsCtx = sws_getContext(src_frame->width, src_frame->height, AVPixelFormat(src_frame->format),
                                 dst_frame->width, dst_frame->height, AVPixelFormat(dst_frame->format),
                                 SWS_POINT, NULL, NULL, NULL);
        if(mSwsCtx == NULL){
            printf("get sws Context failed\n");
            return -1;
        }
        int ret = sws_scale(mSwsCtx, src_frame->data, src_frame->linesize, 0, src_frame->height,
                            dst_frame->data, dst_frame->linesize);
        sws_freeContext(mSwsCtx);
        mSwsCtx = NULL;
        return ret;
    }
    
    int Encoder::encodeOneFrame(uint8_t *buffer, int width, int height, int but_format){
        if(!mIsInit){
            printf("error; please init encoder first.\n");
            return -1;
        }
        av_image_fill_arrays(mFrameYUV->data, mFrameYUV->linesize, buffer,
                             AV_PIX_FMT_YUV420P, mCodecCtx->width, mCodecCtx->height, 1);
        return encodeOneFrame(mFrameYUV);
    }
    
    int Encoder::encodeOneFrame(AVFrame *frameYUV){
        if(!mIsInit){
            printf("error; please init encoder first.\n");
            return -1;
        }
        
        frameYUV->pts = mCurrFrameIdx;
        int got_picture = 0;
        int size = mConfig.width * mConfig.height * 3;
        av_new_packet(mPacket, size);
        int ret = avcodec_encode_video2(mCodecCtx, mPacket, frameYUV, &got_picture);
        if (ret < 0)
        {
            printf("error : failing in encoder...\n");
            av_packet_unref(mPacket);
            return false;
        }
#ifdef _ZH_DEBUG
        printf("avcodec_encode_video2 done %d...\n", mCurrFrameIdx + 1);
#endif
        if (got_picture == 1){
            mPacket->stream_index = mStreamE->index;
            ret = av_write_frame(mFmtCtx, mPacket);
            if(ret < 0){
                printf("error: write frame failed.\n");
            }
#ifdef _ZH_DEBUG
            printf("Writing frame...\n");
#endif
        }
        av_packet_unref(mPacket);
        mCurrFrameIdx++;
        return ret;
    }
    
    int Encoder::encodeFinish(){
        if(!mIsInit){
            printf("error; please init encoder first.\n");
            return -1;
        }
       
        int ret, got_picture;
        while (true)
        {
            int size = mConfig.width * mConfig.height * 3;
            av_new_packet(mPacket, size);
            ret = avcodec_encode_video2(mCodecCtx, mPacket, NULL, &got_picture);
            if (got_picture == 0)
            {
                av_packet_unref(mPacket);
                break;
            }
            mPacket->stream_index = mStreamE->index;
            ret = av_write_frame(mFmtCtx, mPacket);
            if(ret < 0){
                printf("error: write frame failed.\n");
            }
#ifdef _ZH_DEBUG
            printf("Writing frame...\n");
#endif
            av_packet_unref(mPacket);
        }
        
        av_write_trailer(mFmtCtx);
        
        bool isSucc = convertFormat(mH264FilePath.c_str(), mVideoFilePath.c_str());
        if(!isSucc){
            printf("error:convert h264 to mp4 failed.\n");
            return -1;
        }
        remove(mH264FilePath.c_str());
//        mIsInit = false;
        return ret;
    }
    
    bool Encoder::convertFormat(const char* input_h264, const char* output_video)
    {
        AVOutputFormat *ofmt = NULL;
        AVFormatContext *ifmt_ctx_v = NULL, *ofmt_ctx = NULL;
        AVPacket pkt;
        
        int ret, i;
        int videoindex_v = -1, videoindex_out = -1;
        int frame_index = 0;
        
        av_register_all();
        
        // ‰»Î£®Input£©
        if ((ret = avformat_open_input(&ifmt_ctx_v, input_h264, 0, 0)) < 0) {
            printf("Could not open input file.");
            return false;
        }
        if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
            printf("Failed to retrieve input stream information");
            return false;
        }
        
        // ‰≥ˆ£®Output£©
        avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, output_video);
        if (!ofmt_ctx) {
            printf("Could not create output context\n");
            return false;
        }
        ofmt = ofmt_ctx->oformat;
        
        for (i = 0; i < ifmt_ctx_v->nb_streams; i++)
        {
            if (ifmt_ctx_v->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                AVStream *in_stream = ifmt_ctx_v->streams[i];
                AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
                videoindex_v = i;
                if (!out_stream) {
                    printf("Failed allocating output stream\n");
                    return false;
                }
                videoindex_out = out_stream->index;
                //∏¥÷∆AVCodecContextµƒ…Ë÷√£®Copy the settings of AVCodecContext£©
                if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                    printf("Failed to copy context from input to output stream codec context\n");
                    return false;
                }
                out_stream->codec->codec_tag = 0;
                if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                    out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
                break;
            }
        }
        
        //¥Úø™ ‰≥ˆŒƒº˛£®Open output file£©
        if (!(ofmt->flags & AVFMT_NOFILE)) {
            if (avio_open(&ofmt_ctx->pb, output_video, AVIO_FLAG_WRITE) < 0) {
                printf("Could not open output file '%s'", output_video);
                return false;
            }
        }
        //–¥Œƒº˛Õ∑£®Write file header£©
        if (avformat_write_header(ofmt_ctx, NULL) < 0) {
            printf("Error occurred when opening output file\n");
            return false;
        }
        
        AVFormatContext *ifmt_ctx;
        int stream_index = 0;
        AVStream *in_stream, *out_stream;
        ifmt_ctx = ifmt_ctx_v;
        stream_index = videoindex_out;
        
        while (true){
            if(av_read_frame(ifmt_ctx, &pkt) < 0){
                av_packet_unref(&pkt);
                break;
            }
            
            in_stream = ifmt_ctx->streams[pkt.stream_index];
            out_stream = ofmt_ctx->streams[stream_index];
            
            //Write PTS
            AVRational time_base1 = in_stream->time_base;
            //Duration between 2 frames (us)
            int64_t calc_duration = (double)AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
            //Parameters
            pkt.pts = (double)(frame_index*calc_duration) / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            pkt.dts = pkt.pts;
            pkt.duration = (double)calc_duration / (double)(av_q2d(time_base1)*AV_TIME_BASE);
            frame_index++;
            
            /* copy packet */
            //◊™ªªPTS/DTS£®Convert PTS/DTS£©
            pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
            pkt.pos = -1;
            pkt.stream_index = stream_index;
#ifdef _ZH_DEBUG
            printf("Write 1 Packet. size:%5d\tpts:%8d\n", pkt.size, pkt.pts);
#endif
            //–¥»Î£®Write£©
            if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
                printf("Error muxing packet\n");
                av_packet_unref(&pkt);
                break;
            }
            av_packet_unref(&pkt);
        }
        //–¥Œƒº˛Œ≤£®Write file trailer£©
        av_write_trailer(ofmt_ctx);
        avformat_close_input(&ifmt_ctx_v);
        
        /* close output */
        if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
            avio_close(ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);
        
        if (ret < 0 && ret != AVERROR_EOF) {
            printf("Error occurred.\n");
            return false;
        }
        return true;
    }
}
