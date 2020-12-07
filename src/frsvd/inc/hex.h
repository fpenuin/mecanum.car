/*
 * hex.h
 *
 *  Created on: 2016-3-4
 *      Author: daiqingquan502
 */

#ifndef HEX_H_
#define HEX_H_

#include <stdint.h>

#define HEX_OK_FIL (1)
#define HEX_OK_REC (0)
#define HEX_ERR_GEN (-1)
#define HEX_ERR_IO (-2)
#define HEX_ERR_MEM (-3)
#define HEX_ERR_CHK (-4)
#define HEX_ERR_CB	(-5)
#define HEX_ERR_DAT (-6)
#define HEX_ERR_TYP (-7)

typedef int(* HexIoRx_f)(unsigned char *pch);

int hexIoInit(HexIoRx_f get_ch_fun);

int hexFfInit(int len);
void hexFfDeinit(void);

unsigned int hexFfSt(void);
unsigned int hexFfGet(void * buf, unsigned int len);

unsigned int hexRx(int(* cb_fun)(uint32_t addr), int *err);

#endif /* HEX_H_ */
