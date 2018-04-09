//
//  Utils.cpp
//  VideoEncryption
//
//  Created by zhahang on 2018/4/4.
//  Copyright © 2018年 zhahang. All rights reserved.
//

#include "Utils.h"

#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace std;

namespace ZH {
    
    int convert(unsigned char *dst, const unsigned char *src, int length)
    {
        int i;
        for (i = 0; i < length; i++)
        {
            dst[i] = src[i] ^ 0xFF;
        }
        return 0;
    }
    
    int HexXor(unsigned char *array_a, int len_a, unsigned char *array_b, int len_b, unsigned char *array_out){
        if(len_a < 1 || len_b < 1)
            return -1;
        if(array_a == NULL || array_b == NULL || array_out == NULL)
            return -1;
        int len_min = (len_a < len_b) ? len_a : len_b;
        int len_max = (len_a > len_b) ? len_a : len_b;
        
        for (int i = 0; i < len_min; i++) {
            array_out[i] = array_a[i] ^ array_b[i];
        }
        //多出未对应部分直接赋值
        if(len_a > len_b){
            for (int i = len_min; i < len_max; i++)
                array_out[i] = array_a[i];
        }else{
            for (int i = len_min; i < len_max; i++)
                array_out[i] = array_b[i];
        }
        
        return 0;
    }
    
    unsigned long HextoDec(const unsigned char *hex, int length)
    {
        int i;
        unsigned long rslt = 0;
        for (i = 0; i < length; i++){
            rslt += (unsigned long)(hex[i]) << (8 * (length - 1 - i));
        }
        return rslt;
    }
    
    int DectoHex(int dec, unsigned char *hex, int length)
    {
        int i;
        for (i = length - 1; i >= 0; i--)
        {
            hex[i] = (dec % 256) & 0xFF;
            dec /= 256;
        }
        return 0;
    }
    
    unsigned long power(int base, int times)
    {
        int i;
        unsigned long rslt = 1;
        for (i = 0; i < times; i++)
            rslt *= base;
        return rslt;
    }
    
    unsigned long  BCDtoDec(const unsigned char *bcd, int length)
    {
        int i, tmp;
        unsigned long dec = 0;
        for (i = 0; i < length; i++)
        {
            tmp = ((bcd[i] >> 4) & 0x0F) * 10 + (bcd[i] & 0x0F);
            dec += tmp * power(100, length - 1 - i);
        }
        return dec;
    }
    
    int DectoBCD(int Dec, unsigned char *Bcd, int length)
    {
        int i;
        int temp;
        for (i = length - 1; i >= 0; i--)
        {
            temp = Dec % 100;
            Bcd[i] = ((temp / 10) << 4) + ((temp % 10) & 0x0F);
            Dec /= 100;
        }
        return 0;
    }
}



