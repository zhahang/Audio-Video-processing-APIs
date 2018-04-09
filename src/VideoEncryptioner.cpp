//
//  VideoEncryptioner.cpp
//  VideoEncryption
//
//  Created by zhahang on 2018/4/4.
//  Copyright © 2018年 zhahang. All rights reserved.
//

#include "VideoEncryptioner.h"
#include "Utils.h"
#include <fstream>

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

namespace ZH {
    
    int Enctyption(uint8_t *src_data, uint8_t *dst_data, uint8_t *key, int len_key){
        int ret = 0;
        int pos = 0;
        while (pos < 8) {
            dst_data[pos] = src_data[pos];
            pos++;
        }
        
        //第7,8位,为sps长度
        uint8_t *p_sps_size = src_data + 6;
        int sps_size = (int)HextoDec(p_sps_size, 2) + 1;
        uint8_t *sps_data = p_sps_size + 2;
        //接下来2位,为pps长度
        uint8_t *p_pps_size = sps_data + sps_size;
        int pps_size = (int)HextoDec(p_pps_size, 2);
        uint8_t *pps_data = p_pps_size + 2;
        
        uint8_t *xor_sps_data = dst_data + 8;
        ret = HexXor(sps_data, sps_size, key, len_key, xor_sps_data);
        pos += sps_size;
#ifdef _ZH_DEBUG
        printf("xor_sps:");
        for (int i = 0; i < sps_size; i++) {
            printf("%X ",xor_sps_data[i]);
        }
        printf("\n");
#endif
        
        uint8_t *xor_pps_data = xor_sps_data + sps_size + 2;
        ret = HexXor(pps_data, pps_size, key, len_key, xor_pps_data);
        for (int i = 0; i < 2; i++) {
            dst_data[pos+i] = src_data[pos+i];
        }
#ifdef _ZH_DEBUG
        printf("xor_pps:");
        for (int i = 0; i < pps_size; i++) {
            printf("%X ",xor_pps_data[i]);
        }
        printf("\n");
#endif
        return ret;
    }
    
    int VideoEncryption(const char *path){
        int ret = 0;
        av_register_all();
        AVFormatContext *fmtCtx = avformat_alloc_context();
        if ((ret = avformat_open_input(&fmtCtx, path, NULL, NULL)) < 0)
        {
            printf("******** Decode avformat_open_input() Function result=%d\n",ret);
            return ret;
        }
        
        if ((ret = avformat_find_stream_info(fmtCtx, NULL)) < 0)
        {
            printf("******** Decode avformat_find_stream_info() Function result=%d \n",ret);
            avformat_close_input(&fmtCtx);
            return ret;
        }
//        printf("extradata_size=%d\n", fmtCtx->streams[0]->codec->extradata_size);
        int extradata_size = fmtCtx->streams[0]->codec->extradata_size;
        uint8_t *sps_pps_data = new uint8_t[extradata_size];
#ifdef _ZH_DEBUG
        printf("src_data:")
        for (int i=0;i<extradata_size;i++)
            printf("%X ",fmtCtx->streams[0]->codec->extradata[i]);
        printf("\n");
#endif

        for (int i=0;i<extradata_size;i++)
            sps_pps_data[i] = fmtCtx->streams[0]->codec->extradata[i];
        
        uint8_t *key = new uint8_t[6];
        memset(key, 0xFF, 6);
        
        uint8_t *dst_data = new uint8_t[extradata_size];
        ret = Enctyption(sps_pps_data, dst_data, key, 6);
#ifdef _ZH_DEBUG
        printf("dst_data:")
        for (int i = 0; i < extradata_size; i++) {
            printf("%X ",dst_data[i]);
        }
        printf("\n");
#endif
        delete[] key;
        
        std::ifstream inFile;
        long length;
        inFile.open(path);
        inFile.seekg(0, std::ios::end);
        length = inFile.tellg();
        inFile.seekg(0, std::ios::beg);
        uint8_t *buffer = new uint8_t[length];
        inFile.read((char *)buffer, length);
        inFile.close();
        uint8_t *target_data = (uint8_t *)memmem(buffer, length, sps_pps_data, extradata_size);
        for (int i = 0; i < extradata_size; i++) {
            target_data[i] = dst_data[i];
        }
        std::ofstream outFile(path, std::ios::binary);
        outFile.write((char *)buffer, length);
        outFile.close();

        delete[] sps_pps_data;
        delete[] dst_data;
        delete[] buffer;
        return 0;
    };
    
    int VideoDecryption(const char *path){
        return VideoEncryption(path);
    };
}
