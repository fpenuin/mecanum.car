/*
 * pwm.h
 *
 *  Created on: 2018Äê4ÔÂ22ÈÕ
 *      Author: fpenguin
 */

#ifndef APP_PWM_H_
#define APP_PWM_H_

#include <stdint.h>

#define PWM_OUT_CH_0  (0)
#define PWM_OUT_CH_1  (1)
#define PWM_OUT_CH_2  (2)
#define PWM_OUT_CH_3  (3)
#define PWM_OUT_CH_ALL  (255)

typedef struct  __PwmCtrl_t
{   
    uint32_t tsk_period_ms;

    uint32_t out_period_us;
    uint32_t out_pulse_us[4];

}PwmCtrl_t;

extern PwmCtrl_t PwmCtrl;

void pwmTskInit(void);

void pwmOutInit(void);
void pwmOutSetPeriod(uint32_t period_us);
void pwmOutSetPulseCh(uint32_t ch, uint32_t pulse_us);
void pwmOutSetPulse(uint32_t * pulse_us);

int CMD_SetPwmOutPulse(char * arg);
int CMD_SetPwmOutPeriod(char * arg);

int regPwmCmds(void);

#endif /* APP_PWM_H_ */
