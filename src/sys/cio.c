/*
 * cio.c
 *
 *  Created on: 2018Äê4ÔÂ15ÈÕ
 *      Author: fpenguin
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <sys/unistd.h>

#include "../stm32f4.frsvd/dma_uart.h"
#include "../frsvd/inc/fifo.h"


#define USART_STDIN     (0)
#define USART_STDOUT    (0)
#define USART_STDERR    (0)

#define CHECK_CR_BEFORE_LF_IN_STDOUT (0)

#define BUFLEN (128)
static uint8_t __line_data[BUFLEN];
static Fifo_t __line_fifo={__line_data, BUFLEN, 0, 0};

int _write(int fd, char *ptr, int len);
int _read(int fd, char *ptr, int len);

int _write(int fd, char *ptr, int len)
{
    if(!ptr || !len)
    {
        errno=EINVAL;
        return -1;
    }

    uint32_t usart_idx;
    if(fd==STDOUT_FILENO)
        usart_idx=USART_STDOUT;
    else if(fd==STDERR_FILENO)
        usart_idx=USART_STDERR;
    else
    {
        errno = EBADF;
        return -1;
    }

#if CHECK_CR_BEFORE_LF_IN_STDOUT   

    if(ptr[0]=='\n')
    { fDmaUartTx(usart_idx, "\r\n", 2); }
    else
    { fDmaUartTx(usart_idx, ptr, 1); }
    
    uint32_t i,l=(uint32_t)len;
    for(i=1;i<l;i++)
    {
        if((ptr[i]=='\n') && (ptr[i-1]!='\r'))
        { fDmaUartTx(usart_idx, "\r\n", 2); }
        else
        { fDmaUartTx(usart_idx, ptr+i, 1); }
    }

#else

    fDmaUartTx(usart_idx, ptr, (uint16_t)len);

#endif

    return len;
}

int _read(int fd, char *ptr, int len)
{
    if(!ptr)
    {
        errno=EINVAL;
        return -1;
    }

    if(fd!=STDIN_FILENO)
    {
        errno = EBADF;
        return -1;
    }

    static uint32_t got_a_line=0;
    if(got_a_line)
    {
        /*
         * Already got a line buffered, get data from this line,
         * return the actual data received.
         * */
        uint32_t i,l=(uint32_t)len;
        for(i=0;i<l;)
        {
            uint32_t val=fifoGet(&__line_fifo, ptr+i, l-i);
            i+=val;
            if(val==0)
            {
                got_a_line=0;
                break;
            }
        }

        return (int)i;
    }
    else
    {
        /*
         * Line is not complete, get data direct from device,
         * always return 0.
         * */
        uint32_t i,l=(uint32_t)len;
        for(i=0;i<l;i++)
        {
            uint8_t ch;
            if(1==dmaUartRx(USART_STDIN, &ch, 1))
            {
                if(iscntrl(ch))
                {
                    switch(ch)
                    {
                    case '\r':
                        if(2==fifoPut(&__line_fifo, "\r\n", 2))
                        { fDmaUartTx(USART_STDOUT, "\r\n", 2); }
                        got_a_line=1; /*Mark a line even fifo is full.*/
                        break;
                    case '\b':
                        if(__line_fifo.in>__line_fifo.out)
                        {
                            __line_fifo.in--;
                            fDmaUartTx(USART_STDOUT, "\b \b", 3);
                        }
                        else
                        {
                            fDmaUartTx(USART_STDOUT, "\a", 1); /*ring bell*/
                        }    
                        break;
                    }
                }
                else
                {
                    if(1==fifoPut(&__line_fifo, &ch, 1))
                    { fDmaUartTx(USART_STDOUT, &ch, 1); }
                }
            }
            else
                break;
        }

        return 0;
    }
}

