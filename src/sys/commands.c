/*
 * commands.c
 *
 *  Created on: 2018Äê4ÔÂ16ÈÕ
 *      Author: fpenguin
 */

#include "commands.h"

#include <stdio.h>
#include <stdint.h>

#include "../frsvd/inc/cmd.h"

#include "stm32f4xx_hal.h"

int CMD_ClkOut(char * arg);

int CMD_ClkOut(char * arg)
{
    uint32_t en;
    int r = cmdGetArg(arg, &en);
    if(r!=1) return CMD_ERRARG;

    if(en)
    {
        GPIO_InitTypeDef GPIO_InitStruct={0};
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_PLLCLK, RCC_MCODIV_5);
 
    }
    else
    {
        GPIO_InitTypeDef GPIO_InitStruct={0};
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF0_MCO;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    return CMD_SUCCESS;
}

int regSysCmds(void)
{
    int r;
    do
    {
        r = cmdRegister("clkout", CMD_ClkOut,
                "[en]: Enable SYSCLK on PA8 or not."); if(r) break;
    }while(0);

    return r;
}
