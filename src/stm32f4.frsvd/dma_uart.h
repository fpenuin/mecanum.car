/*
 * dma_uart.h
 *
 *  Created on: 2018年4月6日
 *      Author: fpenguin
 */

#ifndef DMA_UART_H
#define DMA_UART_H

#include <stdint.h>

void dmaUartInit(uint32_t idx, uint32_t br, uint16_t rx_buf_size);
void dmaUartDeinit(uint32_t idx);

void fDmaUartTx(uint32_t idx, void * dat, uint16_t len);
uint16_t dmaUartRx(uint32_t idx, void * dat, uint16_t len);
int32_t dmaUartPrintf(uint32_t idx, const char * fmt, ...);

#endif /* DMA_UART_H */
