/*
 * mdebug.h
 *
 *  Created on: 2015-1-4
 *      Author: daiqingquan502
 */

#ifndef MDEBUG_H_
#define MDEBUG_H_

#include <stdint.h>

void memDump(uint32_t start_addr, uint32_t len_in_byte);
int memChk(uint32_t start_addr, uint32_t len_in_byte);

#endif /* MDEBUG_H_ */
