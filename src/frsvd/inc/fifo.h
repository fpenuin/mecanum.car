/*
 * fifo.h
 *
 *  Created on: 2015-4-1
 *      Author: daiqingquan502
 */

#ifndef FIFO_H_
#define FIFO_H_

#include <stdint.h>
#include <stddef.h>

typedef struct _Fifo_t{
	void * buffer;
	uint32_t size;
	uint32_t in;
	uint32_t out;
}Fifo_t;

void fifoInit(void *(* alloc_fun)(size_t size), void (* free_fun)(void *));
Fifo_t * fifoAlloc(uint32_t size);
void fifoFree(Fifo_t * fifo);

uint32_t fifoPut(Fifo_t * fifo, const void * buf, uint32_t len);
uint32_t fifoGet(Fifo_t * fifo, void * buf, uint32_t len);

static inline uint32_t fifoRmn(Fifo_t * fifo)
{
	return fifo->size - (fifo->in - fifo->out);
}
static inline uint32_t fifoLen(Fifo_t * fifo)
{
	return fifo->in - fifo->out;
}
static inline void fifoRst(Fifo_t * fifo)
{
	fifo->in=fifo->out=0;
}

#endif /* FIFO_H_ */
