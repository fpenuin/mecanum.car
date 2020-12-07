/*
 * crc.h
 *
 *  Created on: 2015-3-4
 *      Author: daiqingquan502
 */

#ifndef CRC_H_
#define CRC_H_

#include <stdint.h>

typedef struct _Crc16Param_t{

	uint16_t poly;
	uint16_t rsvd;

	uint16_t init_val;
	uint16_t final_xor;

	uint16_t ref_in;
	uint16_t ref_out;

	uint16_t * table;

}Crc16Param_t;
int32_t crc16TableInit(Crc16Param_t * param);
void crc16TableDeinit(Crc16Param_t * param);
uint16_t crc16Calc(Crc16Param_t * param,
		uint16_t c,
		void * data,
		uint32_t len);

typedef struct _Crc32Param_t{

	uint32_t poly;

	uint32_t init_val;
	uint32_t final_xor;

	uint16_t ref_in;
	uint16_t ref_out;

	uint32_t * table;

}Crc32Param_t;
int32_t crc32TableInit(Crc32Param_t * param);
void crc32TableDeinit(Crc32Param_t * param);
uint32_t crc32Calc(Crc32Param_t * param,
		uint32_t c,
		void * data,
		uint32_t len);


#endif /* CRC_H_ */
