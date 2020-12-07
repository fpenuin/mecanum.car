/*
 * xmodem.h
 *
 *  Created on: 2014-12-15
 *      Author: daiqingquan502
 *  Update: 2016-03-1, dqq
 *  Ôö¼Óº¯ÊýXModemRxE
 */

#ifndef XMODEM_H_
#define XMODEM_H_

typedef int (* IoRxFun_t)(unsigned char * pch);
typedef void (* IoTxFun_t)(unsigned char ch);
typedef unsigned (* IoMsCntFun_t)(void);

int XModemInit(IoTxFun_t send_fun, IoRxFun_t recv_fun, IoMsCntFun_t counter_fun);
int XModemTx(void * buf, unsigned len);
int XModemRx(void * buf, unsigned len);

typedef int (* IoCbFun_t)(unsigned int idx, const void * buf, unsigned int len);
int XModemRxEx(IoCbFun_t cb_fun);

#endif /* XMODEM_H_ */
