/*
 * hex.c
 *
 *  Created on: 2016-3-4
 *      Author: daiqingquan502
 */



#include "../inc/hex.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../inc/fifo.h"

static Fifo_t * __ff=NULL;

static HexIoRx_f __get_ch=NULL;

int hexIoInit(HexIoRx_f get_ch_fun)
{
	if(!get_ch_fun)
		return HEX_ERR_GEN;

	__get_ch=get_ch_fun;
	return 0;
}

int hexFfInit(int len)
{
	fifoInit(malloc, free);
	__ff=fifoAlloc(len);
	if(!__ff)
		return HEX_ERR_MEM;
	else
		return 0;
}
void hexFfDeinit(void)
{
	fifoFree(__ff);
	__ff=NULL;
}
unsigned int hexFfSt(void)
{
	return fifoLen(__ff);
}
unsigned int hexFfGet(void * buf, unsigned int len)
{
	return fifoGet(__ff, buf, len);
}

/*
 * Intel Hexadecimal Object File Format Specification
 *
 * General Record Format:
 * RECORD MARK ':'     RECLEN     LOAD OFFSET   RECTYP      INFO or DATA     CHKSUM
 * 1-byte              1-byte     2-bytes       1-byte      n-bytes          1-byte
 *
 * RECTYP:
 * ¡¯00¡¯ Data Record (8-, 16-, or 32-bit formats)
 * ¡¯01¡¯ End of File Record (8-, 16-, or 32-bit formats)
 * ¡¯02¡¯ Extended Segment Address Record (16- or 32-bit formats)
 * ¡¯03¡¯ Start Segment Address Record (16- or 32-bit formats)
 * ¡¯04¡¯ Extended Linear Address Record (32-bit format only)
 * ¡¯05¡¯ Start Linear Address Record (32-bit format only)
 *
 */

#define TYP_DAT (0x00)
#define TYP_EOF (0x01)
#define TYP_ESA (0x02)
#define TYP_SSA (0x03)
#define TYP_ELA (0x04)
#define TYP_SLA (0x05)

#define MAX_PAYLOAD (0xFF)
#define HDR_LEN (9)
#define CHK_LEN (2)
#define CRLF_LEN (2)

#define __rec_mem_clr() \
	do \
	{ \
		__idx=0; \
		memset(__hdr, 0, HDR_LEN); \
		__offset=0; \
		__len=-1; \
		__type=-1; \
		free(__payload), __payload=NULL; \
	}while(0)
#define __fil_mem_clr() \
	do \
	{ \
		__ela=0; \
		__offset_pre=0; \
		__addr=0; \
		__rec_ctr=0; \
	}while(0);

#ifdef __TMS320C2000__
#define __call_back() \
		do \
		{ \
			unsigned int l1,l2; \
			do \
			{ \
				l1=fifoLen(__ff); \
				if(0!=cb_fun(__addr)) \
					{*err=HEX_ERR_CB;goto exit;} \
				l2=fifoLen(__ff); \
				__addr += ((l1-l2)>>1); \
			}while(l2);\
		}while(0)
#else
#define __call_back() \
		do \
		{ \
			unsigned int l1,l2; \
			do \
			{ \
				l1=fifoLen(__ff); \
				if(0!=cb_fun(__addr)) \
					{*err=HEX_ERR_CB;goto exit;} \
				l2=fifoLen(__ff); \
				__addr += (l1-l2); \
			}while(l2);\
		}while(0)
#endif
unsigned int hexRx(int(* cb_fun)(uint32_t addr), int *err)
{
	/* For record assembling reenter use */
	static int __idx=0;
	static char __hdr[HDR_LEN]={0,0,0,0,0,0,0,0,0};
	static uint16_t __offset=0;
	static int __len=-1;
	static int __type=-1;
	static char * __payload=NULL;

	/* For hex file assembling reenter use */
	static uint16_t __ela=0;
	static uint16_t __offset_pre=0;
	static uint32_t __addr=0;
	static int __rec_ctr=0;

	int ret=__rec_ctr;
	*err=HEX_OK_REC;

	/* Get record head from IO */
	if(__idx<HDR_LEN)
		for(;;)
		{
			unsigned char ch=0;
			if(0==__get_ch(&ch))
			{
				__hdr[__idx]=ch;
				__idx++;
				if(__idx==1)
				{
					if(ch!=':')
						{ *err=HEX_ERR_DAT; goto exit; }
				}
				else
				{
					if(__idx==HDR_LEN)
					{
						int r=sscanf(__hdr, ":%2x%4x%2x", &__len, &__offset, &__type);
						if(r!=3)
							{ *err=HEX_ERR_DAT; goto exit; }
						else
						{
							if(__len>MAX_PAYLOAD)
								{ *err=HEX_ERR_DAT; goto exit; }
							else
								if(__payload)
									{ *err=HEX_ERR_MEM; goto exit; }
								else
								{
									__payload=malloc(__len*2+CHK_LEN);
									if(!__payload)
										{ *err=HEX_ERR_MEM; goto exit; }
									else
										break; //header is all right, go next.
								}
						}
					}
				}
			}
			else
			{
				*err=HEX_ERR_IO;
				return __rec_ctr;
			}
		}

	/* Get payload and checksum */
	for(;;)
	{
		if(__idx < HDR_LEN+__len*2+CHK_LEN+CRLF_LEN)
		{
			unsigned char ch=0;
			if(0==__get_ch(&ch))
			{
				if(__idx < HDR_LEN+__len*2+CHK_LEN)
					__payload[__idx-HDR_LEN]=ch;
				__idx++;
			}
			else
			{
				*err = HEX_ERR_IO;
				return __rec_ctr;
			}
		}
		else
			break;
	}

	/* Calculate checksum from head */
	int r, i, ch, chk; chk=0;
	for(i=1;i<HDR_LEN;i+=2)
	{
		r=sscanf(__hdr+i, "%2x", &ch);
		if(r!=1)
			{*err=HEX_ERR_DAT;goto exit;}
		else
			chk += ch;
	}

	/* Calculate checksum from payload, covert ASCII to bin. */
	char data[255];
	for(i=0;i<__len;i++)
	{
		r=sscanf(__payload+i*2, "%2x", &ch);
		if(r!=1)
			{*err=HEX_ERR_DAT;goto exit;}
		else
		{
			chk += ch;
			data[i]=ch;
		}
	}

	r=sscanf(__payload+i*2, "%2x", &ch);
	if(r!=1)
		{*err=HEX_ERR_DAT;goto exit;}
	else
	{
		int chk_exp=ch;
		if( ((0x100-(chk&0xff))&0xff) != chk_exp )
			{*err=HEX_ERR_CHK;goto exit;}
	}

	/* Data process */
	switch(__type)
	{
	case TYP_DAT:
	{

#ifdef __TMS320C2000__
		if(__offset-__offset_pre!=(__len>>1))
#else
		if(__offset-__offset_pre!=__len)
#endif
		{
			if(fifoLen(__ff))
				__call_back();

			__addr=((uint32_t)__ela<<16)+(uint32_t)__offset;
		}

		if(fifoRmn(__ff) < __len)
			__call_back();

		r = fifoPut(__ff, data, __len);
		if(r!=__len)
			{*err=HEX_ERR_MEM;goto exit;}

		__offset_pre=__offset;

		break;
	}
	case TYP_EOF:
		__call_back();
		*err=HEX_OK_FIL;

		break;
	case TYP_ELA:
	{
		__ela = (((uint32_t)data[0] & 0xff)<<8)
								+
								((uint32_t)data[1] & 0xff);
		if(fifoLen(__ff))
			__call_back();

		__addr=(uint32_t)__ela<<16;
		__offset_pre=0;

		break;
	}
	case TYP_ESA:
	case TYP_SSA:
	case TYP_SLA:
	default:
		*err=HEX_ERR_TYP;
		goto exit;
	}

	__rec_ctr++;
	ret=__rec_ctr;

	__rec_mem_clr();
	if(*err==HEX_OK_FIL)
		__fil_mem_clr();

	return ret;

exit:
	ret=__rec_ctr;

	__rec_mem_clr();
	__fil_mem_clr();

	return ret;
}
