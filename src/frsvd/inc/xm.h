/*
 * xm.h
 *
 *  Created on: 2016-3-4
 *      Author: daiqingquan502
 */

#ifndef XM_H_
#define XM_H_

#include <stdint.h>

typedef int (* XmIoRx_f)(unsigned char * pch);
typedef void (* XmIoTx_f)(unsigned char ch);
typedef unsigned int (* XmIoMsCnt_f)(void);

int xmIoInit(XmIoTx_f send_fun, XmIoRx_f recv_fun, XmIoMsCnt_f cnt_fun);

int xmFfInit(int len);
void xmFfDeinit(void);
uint32_t xmFfSt(void);
uint32_t xmFfGet(void * buf, unsigned len);

#define XM_ERR_NOERR (0)
#define XM_ERR_GEN (-1)
#define XM_ERR_IO (-2)
#define XM_ERR_MEM (-3)
#define XM_ERR_CHK (-4)
#define XM_ERR_CB (-5)
#define XM_ERR_HDR (-5)
#define XM_ERR_ID (-6)
#define XM_ERR_UNREC (-8)
uint32_t xmRx(int(* cb_fun)(void), int * err);
uint32_t xmRxAll(void * buf, unsigned len, int * err);

#endif /* XM_H_ */
