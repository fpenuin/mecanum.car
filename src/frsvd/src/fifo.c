/*
 * fifo.c
 *
 *  Created on: 2015-4-1
 *      Author: daiqingquan502
 *  Update: 2015-08-13, dqq
 *  更改fifoPut和fifoGet中memcpy的调用方式，源指针和目的指针的计算由uint8_t *改为char *，以兼容c2000 MCU。
 */

#include "../inc/fifo.h"

#include <string.h>
#include <stdlib.h>

#define _MIN(x, y)    ((x)<(y) ? (x) : (y))

static void *(* __alloc)(size_t size) = NULL;
static void (* __free)(void *) = NULL;

static inline uint32_t __is_power_of_2(uint32_t n)
{
    return (n&(n-1)) == 0;
}
static inline uint32_t __roundup_power_of_2(uint32_t n)
{
    return n|=n>>16, n|=n>>8, n|=n>>4, n|=n>>2, n|=n>>1, n+1;
}

void fifoInit(void *(* alloc_fun)(size_t size), void (* free_fun)(void *))
{
    __alloc=alloc_fun;
    __free=free_fun;
}

Fifo_t * fifoAlloc(uint32_t size)
{
    void * buf;
    Fifo_t * fifo;

    if(!__is_power_of_2(size))
        size=__roundup_power_of_2(size);

    if(!__alloc) __alloc=malloc;
    if(!__free) __free=free;

    buf = __alloc(size);
    if(!buf)
        return NULL;

    fifo = __alloc(sizeof(Fifo_t));
    if(!fifo)
    {
        __free(buf);
        return NULL;
    }

    fifo->buffer = buf;
    fifo->size = size;
    fifo->in = fifo->out = 0;

    return fifo;
}
void fifoFree(Fifo_t * fifo)
{
    if(!__free) __free=free;

    __free(fifo->buffer);
    __free(fifo);
    fifo=NULL;
}

uint32_t fifoPut(Fifo_t * fifo, const void * buf, uint32_t len)
{
    uint32_t l;

    len = _MIN(len, fifo->size - fifo->in + fifo->out);

    l = _MIN(len, fifo->size - (fifo->in & (fifo->size - 1)));
    memcpy((char *)fifo->buffer + (fifo->in & (fifo->size - 1)), buf, l);
    memcpy((char *)fifo->buffer, (char *)buf + l, len - l);

    fifo->in += len;

    return len;
}
uint32_t fifoGet(Fifo_t * fifo, void * buf, uint32_t len)
{
    uint32_t l;

    len = _MIN(len, fifo->in - fifo->out);

    l = _MIN(len, fifo->size - (fifo->out & (fifo->size - 1)));
    memcpy((char *)buf, (char *)fifo->buffer + (fifo->out & (fifo->size - 1)), l);
    memcpy((char *)buf + l, (char *)fifo->buffer, len - l);

    fifo->out += len;

    return len;
}
