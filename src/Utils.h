//
//  Utils.hpp
//  VideoEncryption
//
//  Created by zhahang on 2018/4/4.
//  Copyright © 2018年 zhahang. All rights reserved.
//

#ifndef _ZH_UTILS_H
#define _ZH_UTILS_H

#include <stdio.h>

namespace ZH {
    /**
     * Invert the data binary
     * @param dst output data
     * @param src input data
     * @param length inout data length
     * @return 0 is success, -1 is fail
     */
    int convert(unsigned char *dst, const unsigned char *src, int length);
    
    /**
     * Two X hexadecimal data for XOR operation
     * @param array_a input hex data A
     * @param len_a data A length
     * @param array_b input hex data B
     * @param len_b data B length
     * @param array_out XOR result data
     * @return 0 is success, -1 is fail
     */
    int HexXor(unsigned char *array_a, int len_a, unsigned char *array_b, int len_b, unsigned char *array_out);
    
    /**
     * convert hex data to dec data
     * @param hex input hex data
     * @param length input data length
     * @return output dec data
     */
    unsigned long HextoDec(const unsigned char *hex, int length);
    
    /**
     * convert dec data to hex data
     * @param dec input dec data
     * @param hex output hex data
     * @param length hex data length
     * @return 0 is success, -1 is fail
     */
    int DectoHex(int dec, unsigned char *hex, int length);
    
    /////////////////////////////////////////////////////////
    //
    //功能：求权
    //
    //输入：int base                    进制基数
    //      int times                   权级数
    //
    //输出：
    //
    //返回：unsigned long               当前数据位的权
    //
    //////////////////////////////////////////////////////////
    unsigned long power(int base, int times);
    
    /////////////////////////////////////////////////////////
    //
    //功能：BCD转10进制
    //
    //输入：const unsigned char *bcd     待转换的BCD码
    //      int length                   BCD码数据长度
    //
    //输出：
    //
    //返回：unsigned long               当前数据位的权
    //
    //思路：压缩BCD码一个字符所表示的十进制数据范围为0 ~ 99,进制为100
    //      先求每个字符所表示的十进制值，然后乘以权
    //////////////////////////////////////////////////////////
    unsigned long  BCDtoDec(const unsigned char *bcd, int length);
    
    /////////////////////////////////////////////////////////
    //
    //功能：十进制转BCD码
    //
    //输入：int Dec                      待转换的十进制数据
    //      int length                   BCD码数据长度
    //
    //输出：unsigned char *Bcd           转换后的BCD码
    //
    //返回：0  success
    //
    //思路：原理同BCD码转十进制
    //
    //////////////////////////////////////////////////////////
    int DectoBCD(int Dec, unsigned char *Bcd, int length);
}




#endif /* Utils_hpp */
