/*
 * shell.c
 *
 *  Created on: 2018年4月16日
 *      Author: fpenguin
 */

#include "shell.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "stm32f4xx_hal.h"

#include "../frsvd/inc/cmd.h"
#include "../stm32f4.frsvd/dma_uart.h"

static uint32_t __enable=0;

void shellChk(void)
{
    if(__enable)
    {
        const char * cmd = cmdGet();
        if(cmd)
            cmdExec(cmd);
    }
}

void shellInit(void)
{
    //设置cio为无缓存模式
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    cmdInit();
}
void shellOn(void)
{
    __enable=1;
    printf("Shell on.\r\n");
}
void shellOff(void)
{
    __enable=0;
    printf("Shell off.\r\n");
}
