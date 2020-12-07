/*
 * dwt_cycle.c
 *
 *  Created on: 2018Äê4ÔÂ23ÈÕ
 *      Author: fpenguin
 */

#include "dwt_cycle.h"

#include "stm32f4xx_hal.h"

#define CYCLE_CNT   (DWT->CYCCNT)

static uint32_t __us_k, __ms_k;

void cycInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    __us_k = SystemCoreClock/1000000;
    __ms_k = SystemCoreClock/1000;
}

void cycDelay(uint32_t cycles)
{
    uint32_t stt = CYCLE_CNT;
    for(;;)
    {
        uint32_t now = CYCLE_CNT;
        if(stt + now > stt)
        {
            if(now - stt > cycles) 
                break;
        }
        else
        {   
            /*This is overflow case*/   
            if(UINT32_MAX - stt + now > cycles)
                break;
        }
    }

}

void cycDelayUs(uint32_t us)
{
    cycDelay(us * __us_k);
}

void cycDelayMs(uint32_t us)
{
    cycDelay(us * __ms_k);
}








