/*
 * memx.h
 *
 *  Created on: 2014-5-20
 *      Author: daiqingquan502
 *  Update: 2015-12-21, dqq
 *  �����Ը��ƣ����28335 char��λ��Ϊ16�����⡣
 */

#ifndef MEMX_H_
#define MEMX_H_

#ifdef __TMS320C2000__

#define MEM64(x)	(*(volatile unsigned long long*)(x))
#define MEM32(x)	(*(volatile unsigned long*)(x))
#define MEM16(x)	(*(volatile unsigned short*)(x))

#else

#define MEM64(x)	(*(volatile unsigned long long*)(x))
#define MEM32(x)	(*(volatile unsigned int*)(x))
#define MEM16(x)	(*(volatile unsigned short*)(x))
#define MEM8(x)		(*(volatile unsigned char*)(x))

#endif

#endif /* MEMX_H_ */
