/*
 * dwt_cycle.h
 *
 *  Created on: 2018Äê4ÔÂ23ÈÕ
 *      Author: fpenguin
 */

#ifndef DWT_CYCLE_H_
#define DWT_CYCLE_H_

#include <stdint.h>

void cycInit(void);
void cycDelay(uint32_t cycles);
void cycDelayUs(uint32_t us);
void cycDelayMs(uint32_t us);

#endif /* DWT_CYCLE_H_ */
