/*
 * xmodem.c
 *
 *  Created on: 2014-12-15
 *      Author: daiqingquan502
 *  Update: 2016-05-29, dqq
 *  增加头文件string.h。
 *  Update: 2015-3-1, dqq
 *  增加函数XModemRxEx。
 *  Update: 2015-3-3, dqq
 *  使用TI c2000 MCU时，__blk_rx中的接收帧计数器为16位，当其超过255时，需要手动置0；
 *  在XModemRxEx与XModemRx中，接收数据错误时，增加清除设备缓存的操作。
 */

#include "../inc/xmodem.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

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

static IoTxFun_t __io_send=NULL;
static IoRxFun_t __io_recv=NULL;
static IoMsCntFun_t __io_counter=NULL;

static inline unsigned char __chksum(void * buf, unsigned len)
{
	if(!buf) return 0;
	unsigned i;
	char sum=0;
	for(i=0;i<len;i++)
		sum += *((char *)buf+i);
	return sum;
}
static inline unsigned short __crc(void * buf, unsigned len)
{

	unsigned char * ptr=buf;
	int crc, i;

	crc=0;
	while(len--)
	{
		crc = crc^ (unsigned short)*ptr++ << 8;
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

static unsigned char __blk_id_exp=1;

static void __blk_rx_rst(void)
{
	__blk_id_exp=1;
}

static int __blk_rx(unsigned to_s, int variant, void * buf)
{
	if( (variant!=XMODEM_CHKSUM) &&
			(variant!=XMODEM_CRC) &&
			(variant!=XMODEM_1K) )
		return -1;
	if(!buf)
		return -1;

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

	unsigned blk_len;
	if(0==rx_ret)
	{
		if(ch==XM_SOH)
			blk_len=128;
		else if(ch==XM_STX)
			blk_len=1024;
		else if(ch==XM_EOT)
			return 1;
		else
			return -2;
	}
	else
		return -1;
	p[0]=ch;

	unsigned i;
	for(i=0;i<2;i++)
	{
		rx_ret=__io_recv(p+1+i);
		if(0!=rx_ret)
			return -1;
	}
	if(p[1]+p[2] != 255)
		return -2;
	if((p[1]!=__blk_id_exp)
			&&
			((p[1]!=__blk_id_exp-1) && __blk_id_exp!=1))
		return -2;

	unsigned len_tmp;
	if(variant==XMODEM_CHKSUM)
		len_tmp = blk_len+1;
	else
		len_tmp = blk_len+2;

	for(i=0;i<len_tmp;i++)
	{
		rx_ret=__io_recv(p+3+i);
		if(0!=rx_ret)
			return -1;
	}

	if(variant==XMODEM_CHKSUM)
	{
		if(p[3+blk_len] != __chksum(p+3, blk_len))
			return -2;
	}
	else
	{
		unsigned short crc = ((unsigned short)p[3+blk_len]<<8) + p[3+blk_len+1];
		if(crc != __crc(p+3, blk_len))
			return -2;
	}

	if(p[1]==__blk_id_exp)
	{
		__blk_id_exp++;
#ifdef __TMS320C2000__
		if(__blk_id_exp>0xff)
			__blk_id_exp=__blk_id_exp-0xff;
#endif
		return 0;
	}
	else
		return 2;
}

int XModemInit(IoTxFun_t send_fun, IoRxFun_t recv_fun, IoMsCntFun_t counter_fun)
{
	if((!send_fun) || (!recv_fun) || (!counter_fun))
		return -1;

	__io_send = send_fun;
	__io_recv = recv_fun;
	__io_counter = counter_fun;
	return 0;
}

int XModemTx(void * buf, unsigned len)
{
	printf("\"%s\" is not supported yet.\r\n", __func__);
	return 0;
}
int XModemRx(void * buf, unsigned len)
{
	if(!buf)
		return -1;

	unsigned char * blk_buf = malloc(3+1024+2);
	if(!blk_buf)
		return -1;

	int ret=0;

	__clr_rx_io();	//Clear the device receiver buffer.
	__blk_rx_rst();	//Reset the block counter.

	do
	{
		/*Initial the transmission by receiver*/
		unsigned rx_err_cnt=0;
		int blk_ret=-1;
		int variant=XMODEM_1K;

		if((variant==XMODEM_CRC) || (variant==XMODEM_1K))
		{
			for(rx_err_cnt=0;rx_err_cnt<MAX_CRC;rx_err_cnt++)
			{
				__io_send(XM_C);
				blk_ret = __blk_rx(10, variant, blk_buf);
				if(0<=blk_ret) break;
			}
			if(0>blk_ret) variant=XMODEM_CHKSUM;
		}
		if(variant==XMODEM_CHKSUM)
		{
			for(rx_err_cnt=0;rx_err_cnt<MAX_ERR;rx_err_cnt++)
			{
				__io_send(XM_NAK);
				blk_ret = __blk_rx(10, variant, blk_buf);
				if(0<=blk_ret) break;
			}
		}
		if(0>blk_ret)
			{ret=-1;break;}
		rx_err_cnt=0;

		/*Process the first block*/
		unsigned idx=0;
		unsigned char * p=buf;
		unsigned blk_len=128;

		if(XM_STX == blk_buf[0])
			blk_len=1024;
		if(blk_len>len)
			{ret=-1;break;}

		memcpy(p+idx, blk_buf+3, blk_len);
		idx += blk_len;
		__io_send(XM_ACK);

		/*Block receive routine*/
		for(;;)
		{
			blk_ret = __blk_rx(1, variant, blk_buf);

			if(0==blk_ret)					//Copy data.
			{
				rx_err_cnt=0;
				if(XM_STX ==  blk_buf[0])
					blk_len=1024;
				else
					blk_len=128;

				if(idx+blk_len>len)
					{ret=-1;break;}

				memcpy(p+idx, blk_buf+3, blk_len);
				idx += blk_len;

				__io_send(XM_ACK);
			}
			else if(1==blk_ret)				//File transmission succeed.
			{
				rx_err_cnt=0;
				__io_send(XM_ACK);
				{ret=idx;break;}
			}
			else if(2==blk_ret) 			//Repeat of the previously block.
			{
				rx_err_cnt=0;
				__io_send(XM_ACK);
			}
			else if(-2==blk_ret) 			//Data corrupted.
			{
				__clr_rx_io();
				rx_err_cnt++;
				__io_send(XM_NAK);
			}
			else
				rx_err_cnt++;

			if(rx_err_cnt>MAX_ERR)
			{ret=-1; break;}
		}

	}while(0);

	unsigned i;
	for(i=0;i<10;i++)
		__io_send(XM_CAN);

	free(blk_buf);

	return ret;
}

int XModemRxEx(IoCbFun_t cb_fun)
{
	if(!cb_fun)
		return -1;

	unsigned char * blk_buf = malloc(3+1024+2);
	if(!blk_buf)
		return -1;

	int ret=0;

	__clr_rx_io();	//Clear the device receiver buffer.
	__blk_rx_rst();	//Reset the block counter.

	do
	{
		/*Initial the transmission by receiver*/
		unsigned rx_err_cnt=0;
		int blk_ret=-1;
		int variant=XMODEM_1K;

		if((variant==XMODEM_CRC) || (variant==XMODEM_1K))
		{
			for(rx_err_cnt=0;rx_err_cnt<MAX_CRC;rx_err_cnt++)
			{
				__io_send(XM_C);
				blk_ret = __blk_rx(10, variant, blk_buf);
				if(0<=blk_ret) break;
			}
			if(0>blk_ret) variant=XMODEM_CHKSUM;
		}
		if(variant==XMODEM_CHKSUM)
		{
			for(rx_err_cnt=0;rx_err_cnt<MAX_ERR;rx_err_cnt++)
			{
				__io_send(XM_NAK);
				blk_ret = __blk_rx(10, variant, blk_buf);
				if(0<=blk_ret) break;
			}
		}
		if(0>blk_ret)
			{ret=-1;break;}
		rx_err_cnt=0;

		/*Process the first block*/
		unsigned idx=0;
		unsigned blk_len=128;

		if(XM_STX == blk_buf[0])
			blk_len=1024;

		if(0!=cb_fun(idx, blk_buf+3, blk_len))
			{ret=-3;break;}
		idx += blk_len;
		__io_send(XM_ACK);

		/*Block receive routine*/
		for(;;)
		{
			blk_ret = __blk_rx(1, variant, blk_buf);

			if(0==blk_ret)					//Copy data.
			{
				rx_err_cnt=0;
				if(XM_STX ==  blk_buf[0])
					blk_len=1024;
				else
					blk_len=128;

				if(0!=cb_fun(idx, blk_buf+3, blk_len))
					{ret=-3;break;}
				idx += blk_len;

				__io_send(XM_ACK);
			}
			else if(1==blk_ret)				//File transmission succeed.
			{
				rx_err_cnt=0;
				__io_send(XM_ACK);
				{ret=idx;break;}
			}
			else if(2==blk_ret) 			//Repeat of the previously block.
			{
				rx_err_cnt=0;
				__io_send(XM_ACK);
			}
			else if(-2==blk_ret) 			//Data corrupted.
			{
				__clr_rx_io();
				rx_err_cnt++;
				__io_send(XM_NAK);
			}
			else
				rx_err_cnt++;

			if(rx_err_cnt>MAX_ERR)
			{ret=-1; break;}
		}

	}while(0);

	unsigned i;
	for(i=0;i<10;i++)
		__io_send(XM_CAN);

	free(blk_buf);

	return ret;
}











