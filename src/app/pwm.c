/*
 * pwm.c
 *
 *  Created on: 2018年4月22日
 *      Author: fpenguin
 */


#include "pwm.h"

#include <stdio.h>
#include <assert.h>

#include "stm32f4xx_hal.h"

#include "../frsvd/inc/cmd.h"
#include "../frsvd/inc/tsk.h"



static TIM_HandleTypeDef __output_tim;

PwmCtrl_t PwmCtrl;
TskHandle_t htskPwmChk=NULL;

static uint32_t __cnt_clk_mhz=2;

static void __pwm_main(void);
static void __pwm_main(void)
{
    pwmOutSetPeriod(PwmCtrl.out_period_us);
    pwmOutSetPulse(PwmCtrl.out_pulse_us);
}

void pwmTskInit(void)
{
    pwmOutInit();

    htskPwmChk = tskAdd("GamePadChk", __pwm_main, 0,0);
    if(!htskPwmChk)
    {
        printf("%s, line%u: Fail to create pwm main task!!!\r\n", \
        __func__, __LINE__);
    }

    tskPeriod(htskPwmChk, PwmCtrl.tsk_period_ms);
}


void pwmOutInit(void)
{   
    uint32_t tim_clk_mhz, prescaler;

    tim_clk_mhz = HAL_RCC_GetPCLK1Freq();
    tim_clk_mhz = tim_clk_mhz/1000000*2; 
    prescaler = tim_clk_mhz/__cnt_clk_mhz-1;

    printf("%s: pwm output timer clk_in = %lu mHz, clk_out = %.2f mHz\r\n",
            __func__, tim_clk_mhz, (float)tim_clk_mhz/(float)(prescaler+1));

    __output_tim.Instance               = TIM4;
    __output_tim.Init.CounterMode       = TIM_COUNTERMODE_UP;
    __output_tim.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    __output_tim.Init.Prescaler         = prescaler;
    __output_tim.Init.Period            = 0;    /*每个周期的计数，初始值设为0*/
    HAL_TIM_OC_Init(&__output_tim);

    TIM_OC_InitTypeDef TIM_OC_InitStuct={0};
    TIM_OC_InitStuct.OCMode         = TIM_OCMODE_PWM1;
    TIM_OC_InitStuct.Pulse          = 0;        /*比较器的计数，初始值设为0*/
    TIM_OC_InitStuct.OCPolarity     = TIM_OCPOLARITY_HIGH;
    TIM_OC_InitStuct.OCFastMode     = TIM_OCFAST_ENABLE;
    HAL_TIM_PWM_ConfigChannel(&__output_tim, &TIM_OC_InitStuct, TIM_CHANNEL_1);
    HAL_TIM_PWM_ConfigChannel(&__output_tim, &TIM_OC_InitStuct, TIM_CHANNEL_2);
    HAL_TIM_PWM_ConfigChannel(&__output_tim, &TIM_OC_InitStuct, TIM_CHANNEL_3);
    HAL_TIM_PWM_ConfigChannel(&__output_tim, &TIM_OC_InitStuct, TIM_CHANNEL_4);

    HAL_TIM_PWM_Start(&__output_tim, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&__output_tim, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&__output_tim, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&__output_tim, TIM_CHANNEL_4);
}
void pwmOutSetPeriod(uint32_t period_us)
{
    __HAL_TIM_SET_AUTORELOAD(&__output_tim, __cnt_clk_mhz*period_us);
}
void pwmOutSetPulseCh(uint32_t ch, uint32_t pulse_us)
{
    switch (ch)
    {
    case 0:
        __HAL_TIM_SET_COMPARE(&__output_tim, TIM_CHANNEL_1, \
                                pulse_us*__cnt_clk_mhz);
        break;
    case 1:
        __HAL_TIM_SET_COMPARE(&__output_tim, TIM_CHANNEL_2, \
                                pulse_us*__cnt_clk_mhz);
        break;
    case 2:
        __HAL_TIM_SET_COMPARE(&__output_tim, TIM_CHANNEL_3, \
                                pulse_us*__cnt_clk_mhz);
        break;
    case 3:
        __HAL_TIM_SET_COMPARE(&__output_tim, TIM_CHANNEL_4, \
                                pulse_us*__cnt_clk_mhz);
        break;
    default:
        ;
    }
    
}
void pwmOutSetPulse(uint32_t * pulse_us)
{
    assert(pulse_us);

    __HAL_TIM_SET_COMPARE(&__output_tim, 
                            TIM_CHANNEL_1, *(pulse_us+0)*__cnt_clk_mhz);
    __HAL_TIM_SET_COMPARE(&__output_tim, 
                            TIM_CHANNEL_2, *(pulse_us+1)*__cnt_clk_mhz);
    __HAL_TIM_SET_COMPARE(&__output_tim, 
                            TIM_CHANNEL_3, *(pulse_us+2)*__cnt_clk_mhz);                            
    __HAL_TIM_SET_COMPARE(&__output_tim, 
                            TIM_CHANNEL_4, *(pulse_us+3)*__cnt_clk_mhz);
    
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

int CMD_SetPwmOutPulse(char * arg)
{
    uint32_t ch, us;
    int r = cmdGetArg(arg, &ch, &us);
    if(r!=2) return CMD_ERRARG;
    if(ch>=4) return CMD_ERRARG;

    PwmCtrl.out_pulse_us[ch] = us;

    return CMD_SUCCESS;
}
int CMD_SetPwmOutPeriod(char * arg)
{
    uint32_t us;
    int r = cmdGetArg(arg, &us);
    if(r!=1) return CMD_ERRARG;

    PwmCtrl.out_period_us = us;

    return CMD_SUCCESS;
}

#pragma GCC diagnostic pop

int regPwmCmds(void)
{
    int r;
    do
    {
        r = cmdRegister("poperiod", CMD_SetPwmOutPeriod, \
                "[us]: Set output PWM period by us."); if(r) break;

        r = cmdRegister("populse", CMD_SetPwmOutPulse, \
                "[ch],[us]: Set output PWM pulse by ch and us."); if(r) break;

    }while(0);

    return r;
}