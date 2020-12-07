/*
 * xm.c
 *
 *  Created on: 2016-3-4
 *      Author: daiqingquan502
 */

#include "../inc/xm.h"

#include <stdlib.h>

#define MAX_ERR		(10)
#define MAX_CRC		(3)

#define CLR_TO		(1024)

#define XMODEM_CHKSUM	(0)
#define XMODEM_CRC		(1)
#define XMODEM_1K		(2)

#define XM_SOH	(0x01)
#define XM_STX	(0x02)
#define XM_EOT	(0x04)
#define XM_ACK	(0x06)
#define XM_NAK	(0x15)
#define XM_CAN	(0x18)
#define XM_C	(0x43)

static XmIoTx_f __io_send=NULL;
static XmIoRx_f __io_recv=NULL;
static XmIoMsCnt_f __io_counter=NULL;

static inline unsigned char __chksum(void * buf, unsigned len)
{
	if(!buf) return 0;
	unsigned i;
	char sum=0;
	for(i=0;i<len;i++)
		sum += *((char *)buf+i);
	return sum&0xff;
}
static inline uint16_t __crc(void * buf, unsigned len)
{

	unsigned char * ptr=buf;
	int crc, i;

	crc=0;
	while(len--)
	{
		crc = crc^ (uint16_t)*ptr++ << 8;
		for(i=0; i<8; i++)
			if(crc & 0x8000)
				crc=crc<<1^0x1021;
			else
				crc=crc<<1;
	}

	return (crc & 0xFFFF);
}

static int __clr_rx_io(void)
{
	int rx_ret, i;
	unsigned char ch;
	for(i=0;i<CLR_TO;i++)
	{
		rx_ret = __io_recv(&ch);
		if(rx_ret<0)
			break;
	}

	if(i==CLR_TO)
		return -1;
	else
		return 0;
}

static unsigned char __pkt_id_exp=1;
static inline void __pkt_rx_rst(void)
{
	__pkt_id_exp=1;
}

#define XM_PKT_RPT (2)
#define XM_PKT_EOT (1)
#define XM_PKT_OK (0)
static int __pkt_rx(unsigned to_s, int variant, void * buf)
{
	if( (variant!=XMODEM_CHKSUM) &&
			(variant!=XMODEM_CRC) &&
			(variant!=XMODEM_1K) )
		return XM_ERR_GEN;
	if(!buf)
		return XM_ERR_GEN;

	unsigned ms=__io_counter();
	unsigned to=to_s*1000;
	unsigned char ch=0;
	int rx_ret;
	unsigned char * p = buf;

	for(;;)
	{
		rx_ret=__io_recv(&ch);
		if(0==rx_ret) break;
		if(__io_counter()-ms>to) break;
	}

	unsigned pkt_len;
	if(0==rx_ret)
	{
		if(ch==XM_SOH)
			pkt_len=128;
		else if(ch==XM_STX)
			pkt_len=1024;
		else if(ch==XM_EOT)
			return XM_PKT_EOT;
		else
			return XM_ERR_HDR;
	}
	else
		return XM_ERR_IO;
	p[0]=ch;

	unsigned i;
	for(i=0;i<2;i++)
	{
		rx_ret=__io_recv(p+1+i);
		if(0!=rx_ret)
			return XM_ERR_IO;
	}
	if(p[1]+p[2] != 255)
		return XM_ERR_ID;
	if((p[1]!=__pkt_id_exp)
			&&
			((p[1]!=__pkt_id_exp-1) && __pkt_id_exp!=1))
		return XM_ERR_ID;

	unsigned len_tmp;
	if(variant==XMODEM_CHKSUM)
		len_tmp = pkt_len+1;
	else
		len_tmp = pkt_len+2;

	for(i=0;i<len_tmp;i++)
	{
		rx_ret=__io_recv(p+3+i);
		if(0!=rx_ret)
			return XM_ERR_IO;
	}

	if(variant==XMODEM_CHKSUM)
	{
		if(p[3+pkt_len] != __chksum(p+3, pkt_len))
			return XM_ERR_CHK;
	}
	else
	{
		uint16_t crc = ((uint16_t)p[3+pkt_len]<<8) + p[3+pkt_len+1];
		if(crc != __crc(p+3, pkt_len))
			return XM_ERR_CHK;
	}

	if(p[1]==__pkt_id_exp)
	{
		__pkt_id_exp++;
		__pkt_id_exp&=0xff;
		return XM_PKT_OK;
	}
	else
		return XM_PKT_RPT;
}

int xmIoInit(XmIoTx_f send_fun, XmIoRx_f recv_fun, XmIoMsCnt_f cnt_fun)
{
	if((!send_fun) || (!recv_fun) || (!cnt_fun))
		return XM_ERR_GEN;

	__io_send = send_fun;
	__io_recv = recv_fun;
	__io_counter = cnt_fun;
	return XM_ERR_NOERR;
}
uint32_t xmRxAll(void * buf, unsigned len, int * err)
{
	return XM_ERR_NOERR;
}

#include "../inc/fifo.h"

#include <stdint.h>
#include <string.h>

static Fifo_t * __ff=NULL;

int xmFfInit(int len)
{
	fifoInit(malloc, free);
	__ff=fifoAlloc(len);
	if(!__ff)
		return XM_ERR_MEM;
	else
		return XM_ERR_NOERR;
}
void xmFfDeinit(void)
{
	fifoFree(__ff);
	__ff=NULL;
}
uint32_t xmFfSt(void)
{
	return fifoLen(__ff);
}
uint32_t xmFfGet(void * buf, unsigned len)
{
	return fifoGet(__ff, buf, len);
}
uint32_t xmRx(int(* cb_fun)(void), int * err)
{
	uint32_t idx=0;
	*err=XM_ERR_NOERR;

	if(!cb_fun)
		{*err=XM_ERR_GEN; return idx;}

	char * blk_buf = malloc(3+1024+2);
	if(!blk_buf)
		{*err=XM_ERR_MEM; return idx;}

	__clr_rx_io();	//Clear the device receiver buffer.
	__pkt_rx_rst();	//Reset the block counter.

	/*Initial the transmission by receiver*/
	unsigned rx_err_cnt=0;
	int pkt_ret;
	int variant=XMODEM_1K;

	if((variant==XMODEM_CRC) || (variant==XMODEM_1K))
	{
		for(rx_err_cnt=0;rx_err_cnt<MAX_CRC;rx_err_cnt++)
		{
			__io_send(XM_C);
			pkt_ret = __pkt_rx(10, variant, blk_buf);
			if(XM_PKT_OK==pkt_ret)
				break;
			else if(XM_ERR_IO==pkt_ret)
				continue;
			else
			{*err=pkt_ret;return idx;}
		}
		if(rx_err_cnt==MAX_CRC)
			variant=XMODEM_CHKSUM;
	}
	if(variant==XMODEM_CHKSUM)
	{
		for(rx_err_cnt=0;rx_err_cnt<MAX_ERR;rx_err_cnt++)
		{
			__io_send(XM_NAK);
			pkt_ret = __pkt_rx(10, variant, blk_buf);
			if(XM_ERR_NOERR==pkt_ret) break;
		}
	}
	if(XM_ERR_NOERR!=pkt_ret)
		{*err=pkt_ret;goto exit;}
	rx_err_cnt=0;

	/*Process the first block*/
	uint32_t pkt_len;pkt_len=128;
	uint32_t ff_ret;ff_ret=0;

	if(XM_STX == blk_buf[0])
		pkt_len=1024;

	ff_ret = fifoPut(__ff, blk_buf+3, pkt_len);
	if(ff_ret!=pkt_len)
		{*err=XM_ERR_MEM;goto exit;}
	idx += pkt_len;
	__io_send(XM_ACK);

	/*Block receive routine*/
	for(;;)
	{
		pkt_ret = __pkt_rx(1, variant, blk_buf);

		if(XM_PKT_OK==pkt_ret)					//Copy data.
		{
			rx_err_cnt=0;
			if(XM_STX ==  blk_buf[0])
				pkt_len=1024;
			else
				pkt_len=128;

			while(fifoRmn(__ff)<pkt_len)
				if(0!=cb_fun())
					{*err=XM_ERR_CB;goto exit;}

			ff_ret = fifoPut(__ff, blk_buf+3, pkt_len);
			if(ff_ret!=pkt_len)
				{*err=XM_ERR_MEM;goto exit;}
			idx += pkt_len;
			__io_send(XM_ACK);

		}
		else if(XM_PKT_EOT==pkt_ret)			//File transmission succeed.
		{
			rx_err_cnt=0;
			__io_send(XM_ACK);

			while(fifoLen(__ff))
				if(0!=cb_fun())
					{*err=XM_ERR_CB;goto exit;}

			goto exit;
		}
		else if(XM_PKT_RPT==pkt_ret) 			//Repeat of the previously block.
		{
			rx_err_cnt=0;
			__io_send(XM_ACK);
		}
		else if(XM_ERR_CHK==pkt_ret) 			//Data corrupted.
		{
			__clr_rx_io();
			rx_err_cnt++;
			__io_send(XM_NAK);
		}
		else
			rx_err_cnt++;

		if(rx_err_cnt>MAX_ERR)
			{*err=XM_ERR_UNREC; goto exit;}
	}

exit:
	do
	{
		unsigned i;
		for(i=0;i<10;i++)
			__io_send(XM_CAN);
	}while(0);

	free(blk_buf);

	return idx;
}



