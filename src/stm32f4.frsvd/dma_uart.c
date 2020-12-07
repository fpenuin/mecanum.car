/*
 * dma_uart.c
 *
 *  Created on: 2018Äê4ÔÂ6ÈÕ
 *      Author: fpenguin
 */

#include "dma_uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "stm32f4xx_hal.h"
#include "../frsvd/inc/fifo.h"

#define PRINTF_BUF_SIZE (128)
#define USART_NUM       (8)

#ifndef USART1
#define USART1 (0)
#endif
#ifndef USART2
#define USART2 (0)
#endif
#ifndef USART3
#define USART3 (0)
#endif
#ifndef USART4
#define USART4 (0)
#endif
#ifndef USART5
#define USART5 (0)
#endif
#ifndef USART6
#define USART6 (0)
#endif
#ifndef USART7
#define USART7 (0)
#endif
#ifndef USART8
#define USART8 (0)
#endif

static USART_TypeDef * __uart_reg_base[USART_NUM]={USART1,USART2,USART3,USART4,
        USART5,USART6,USART7,USART8};

static UART_HandleTypeDef * __huart[USART_NUM]={0};

static Fifo_t * __ff_rx[USART_NUM]={0};

static void fifoPut_move_ptr_only(Fifo_t * ff, uint32_t rmn)
{
    uint32_t now = ff->size - rmn;
    uint32_t in = ff->in & (ff->size -1);

    if(now>in)
    {
        ff->in += (now-in);
    }
    else
    {
        ff->in += (ff->size - (in - now));
    }
}

void dmaUartInit(uint32_t idx, uint32_t br, uint16_t rx_buf_size)
{
    assert(idx<USART_NUM);

    if(NULL==__huart[idx])
    {
        __huart[idx] = malloc(sizeof(UART_HandleTypeDef));
        assert(__huart);
        memset(__huart[idx], 0, sizeof(UART_HandleTypeDef));
    }

    __huart[idx]->Instance           = __uart_reg_base[idx];
    __huart[idx]->Init.BaudRate      = br;
    __huart[idx]->Init.WordLength    = UART_WORDLENGTH_8B;
    __huart[idx]->Init.StopBits      = UART_STOPBITS_1;
    __huart[idx]->Init.Parity        = UART_PARITY_NONE;
    __huart[idx]->Init.HwFlowCtl     = UART_HWCONTROL_NONE;
    __huart[idx]->Init.Mode          = UART_MODE_TX_RX;
    __huart[idx]->Init.OverSampling  = UART_OVERSAMPLING_16;
    HAL_UART_Init(__huart[idx]);

    if(NULL==__ff_rx[idx])
    {
        __ff_rx[idx]=fifoAlloc(rx_buf_size);
        assert(__ff_rx[idx]);
    }
    
    HAL_UART_Receive_DMA(__huart[idx], __ff_rx[idx]->buffer, (uint16_t)__ff_rx[idx]->size);
}
void dmaUartDeinit(uint32_t idx)
{
    assert(idx<USART_NUM);
    
    if(__huart[idx])
    { free(__huart[idx]), __huart[idx]=NULL; }

    if(__ff_rx[idx])
    { fifoFree(__ff_rx[idx]), __ff_rx[idx]=NULL; }
}

void fDmaUartTx(uint32_t idx, void * dat, uint16_t len)
{
    assert(idx<USART_NUM);

    for(HAL_StatusTypeDef state=HAL_BUSY; state!=HAL_OK; )
    { state = HAL_UART_Transmit_DMA(__huart[idx], dat, len); }
}
uint16_t dmaUartRx(uint32_t idx, void * dat, uint16_t len)
{
    assert(idx<USART_NUM);

    uint32_t tmp = fifoGet(__ff_rx[idx], dat, len);
    return (uint16_t)tmp;
}
int32_t dmaUartPrintf(uint32_t idx, const char * fmt, ...)
{
    assert(idx<USART_NUM);

    char * buf=malloc(PRINTF_BUF_SIZE);
    assert(buf);

    va_list ap;
    va_start(ap, fmt);
    int32_t rpf=vsnprintf(buf, PRINTF_BUF_SIZE, fmt, ap);
    va_end(ap);

    if(rpf<0)
    {
        free(buf);
        return rpf;
    }
    else
    {
        fDmaUartTx(idx, buf, (uint16_t)rpf);
        free(buf);
        return rpf;
    }
}

/*********************************************************************/

/******************UART1 Interrupt configuration**********************/

/*
  Only enable UART IDLE(aka. time-out) interrupt.
*/
void USART1_IRQHandler(void);
void USART1_IRQHandler(void)
{
  /* ST's HAL_UART_IRQHandler() does not handler the IDLE interrupt. */
  /* HAL_UART_IRQHandler(__huart[0]); */
  
  if(RESET != __HAL_UART_GET_IT_SOURCE(__huart[0], UART_IT_IDLE))
  {
    __HAL_UART_CLEAR_IDLEFLAG(__huart[0]);
    
    /* 
      If the rx line is idle at N * half_rx_buf_size, you will receive 
      both UART IDLE and DMA RX(HALF)CPLT interrupt. 
    */
    uint32_t item_left = __HAL_DMA_GET_COUNTER(__huart[0]->hdmarx);
    uint32_t rx_buf_size = __huart[0]->RxXferSize;
    if( (item_left!=rx_buf_size) \
            && \
            (item_left!=(rx_buf_size>>1)) )
    {
      fifoPut_move_ptr_only(__ff_rx[0], __HAL_DMA_GET_COUNTER(__huart[0]->hdmarx));
    }
  }

}

/* 
  UART DMA rx complete proccess:
  move the write ptr of the globla rx fifo.
*/
void DMA2_Stream5_IRQHandler(void);
void DMA2_Stream5_IRQHandler(void)
{
  HAL_DMA_IRQHandler(__huart[0]->hdmarx);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  uint32_t idx;
  for(idx=0;idx<USART_NUM;idx++)
  {
    if(huart->Instance == __uart_reg_base[idx])
        break;
  }
  fifoPut_move_ptr_only(__ff_rx[idx], 0);
}
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
  uint32_t idx;
  for(idx=0;idx<USART_NUM;idx++)
  {
    if(huart->Instance == __uart_reg_base[idx])
        break;
  }
  fifoPut_move_ptr_only(__ff_rx[idx], __ff_rx[idx]->size>>1);
}

/* 
  UART DMA tx complete proccess: 
  Just set flag to tell user, DMA is ready for next transmit.
  I modified the static func UART_DMATransmitCplt() in the stm32f4xx_hal_uart.c.
*/
void DMA2_Stream7_IRQHandler(void);
void DMA2_Stream7_IRQHandler(void)
{
  HAL_DMA_IRQHandler(__huart[0]->hdmatx);
}

