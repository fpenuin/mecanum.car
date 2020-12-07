/*
 * crc.c
 *
 *    Created on: 2015-3-4
 *      Author: daiqingquan502
 *  Update: 2015-12-21, dqq
 *  兼容性改善，解决28335 char型位宽为16的问题。
 */

#include "../inc/crc.h"

#include <stdlib.h>

#ifdef __TMS320C2000__
#define uint8_t unsigned char
#define int8_t char
#endif

static uint8_t __w8_reflect(uint8_t ch)
{
    ch = (uint8_t)(((ch &0x55) <<1) | ((ch >>1) &0x55));
    ch = (uint8_t)(((ch &0x33) <<2) | ((ch >>2) &0x33));
    ch = (uint8_t)(((ch &0x0F) <<4) | ((ch >>4) &0x0F));
    return ch;
}
static uint16_t __w16_reflect(uint16_t v)
{
    v = (uint16_t)(((v &0x5555) <<1) | ((v >>1) &0x5555));
    v = (uint16_t)(((v &0x3333) <<2) | ((v >>2) &0x3333));
    v = (uint16_t)(((v &0x0f0f) <<4) | ((v >>4) &0x0f0f));
    v = (uint16_t)(((v &0x00ff) <<8) | ((v >>8) &0x00ff));
    return v;
}
static uint32_t __w32_reflect(uint32_t v)
{
    v = (uint32_t)(((v &0x55555555) <<1) | ((v >>1) &0x55555555));
    v = (uint32_t)(((v &0x33333333) <<2) | ((v >>2) &0x33333333));
    v = (uint32_t)(((v &0x0f0f0f0f) <<4) | ((v >>4) &0x0f0f0f0f));
    v = (uint32_t)(((v &0x00ff00ff) <<8) | ((v >>8) &0x00ff00ff));
    v = (uint32_t)(((v &0x0000ffff) <<16) | ((v >>16) &0x0000ffff));
    return v;
}

int32_t crc16TableInit(Crc16Param_t * param)
{
    uint16_t remainder;
    uint16_t dividend;
    uint16_t bit;

    param->table = malloc(2*256);
    if(!param->table) return -1;

    for(dividend=0; dividend<256; dividend++)
    {
        remainder = (uint16_t)(dividend<<8);

        for(bit=0; bit<8; bit++)
            if( remainder & 0x8000 )
                remainder = (uint16_t)(remainder<<1) ^ param->poly;
            else
                remainder = (uint16_t)(remainder<<1);

        param->table[dividend] = remainder;
    }

    return 0;
}
int32_t crc32TableInit(Crc32Param_t * param)
{
    uint32_t remainder;
    uint32_t dividend;
    uint32_t bit;

    param->table = malloc(4*256);
    if(!param->table) return -1;

    for(dividend=0; dividend<256; dividend++)
    {
        remainder = dividend << 24;

        for(bit=0; bit<8; bit++)
            if( remainder & 0x80000000 )
                remainder = (remainder<<1) ^ param->poly;
            else
                remainder = remainder<<1;

        param->table[dividend] = remainder;
    }

    return 0;
}

void crc16TableDeinit(Crc16Param_t * param)
{
    if(param->table)
    {
        free(param->table);
        param->table=NULL;
    }
}
void crc32TableDeinit(Crc32Param_t * param)
{
    if(param->table)
    {
        free(param->table);
        param->table=NULL;
    }
}

uint16_t crc16Calc(Crc16Param_t * param,
        uint16_t c,
        void * data,
        uint32_t len)
{
    uint8_t byte, tmp;
    uint16_t remainder = c;

    uint32_t offset;
    for(offset=0; offset<len; offset++)
    {
        tmp = *((uint8_t *)data+offset);
        if(param->ref_in)
            tmp = __w8_reflect(tmp);

        byte = (uint8_t)(remainder>>8) ^ tmp;
        remainder = param->table[byte] ^ (uint16_t)(remainder<<8);
    }

    if(!data)
    {
        if(param->ref_out)
            remainder = __w16_reflect(remainder);

        return remainder ^ param->final_xor;
    }
    else
        return remainder;
}
uint32_t crc32Calc(Crc32Param_t * param,
        uint32_t c,
        void * data,
        uint32_t len)
{
    uint8_t byte, tmp;
    uint32_t remainder = c;

    uint32_t offset;
    for(offset=0; offset<len; offset++)
    {
        tmp = *((uint8_t *)data+offset);
        if(param->ref_in)
            tmp = __w8_reflect(tmp);

        byte = (uint8_t)(remainder>>24) ^ tmp;
        remainder = param->table[byte] ^ (uint32_t)(remainder<<8);
    }

    if(!data)
    {
        if(param->ref_out)
            remainder = __w32_reflect(remainder);

        return remainder ^ param->final_xor;
    }
    else
        return remainder;
}
