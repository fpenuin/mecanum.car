/*
 * controller.c
 *
 *  Created on: 2020��10��31��
 *      Author: fpenguin
 */

#include "controller.h"

#include <stdio.h>
#include <math.h>

#include "stm32f4xx_hal.h"
#include "arm_math.h"

#include "../app/pwm.h"
#include "../app/gamepad.h"
#include "../app/mecanum.h"


/* �������ݽṹ���и������ݵ�ȡֵ **********************************/

/* ��תʱ������ǰ���ƶ�������ΪWHEELS_AXIS_X��
    ��תʱ������ǰ���ƶ�����ֵΪWHEELS_AXIS_Y�� */
#define WHL_AXIS_X (0)
#define WHL_AXIS_Y (1)

/* �����±������ӵĶ�Ӧ��ϵ */
#define WHL_IDX_FRONT_LEFT   (0)
#define WHL_IDX_FRONT_RIGHT  (1)
#define WHL_IDX_BACK_LEFT    (2)
#define WHL_IDX_BACK_RIGHT   (3)


/* ������������ֲ���***********************************************/
#define PWM_OUT_TSK_PERIOD_MS   (10)
#define PWM_OUT_PULSE_MID       (1500)
#define PWM_OUT_PERIOD_US       (20000)


#define GAMEPAD_CHK_INVL_MS (10)
#define GAMEPAD_VIB_LAST_MS (100)

typedef struct __Wheel_t{

    uint32_t axis;
    uint32_t pwm_ch;

    /*
        ����������������У׼�������֤�������ת����ͬ��С����ֱ�ߡ�

        data_calibrated =
            (data_raw * cali_mul) >> cali_shift_right + cali_offset

        ʹ������У׼�� y=ax+b�� Ϊ�������Ч�ʣ�ϵ��a�����Ϊ cali_mul ��
        cali_shift_right��һ��cali_shift_rightȡ7����a=cali_mul/128��
    */
    int32_t cali_mul;
    int32_t cali_shift_right;
    int32_t cali_offset;

    float cali_mul_f;
    float cali_offset_f;

    float out_percentage;   /*-1.0~1.0*/

    uint32_t out_pulse_us;
    int32_t out_pulse_us_max;
    int32_t out_pulse_us_min;

}Wheel_t;

typedef struct __ProcdGamepadData_t{
    float LX;
    float LY;
    float RX;
    float RY;

    float RIGHT;
    float LEFT;
    float UP;
    float DOWN;

    float TRIANGLE;
    float CIRCLE;
    float CROSS;
    float SQUARE;

    float L1;
    float L2;
    float R1;
    float R2;
}ProcdGamepadData_t;

typedef struct __Mcb_t{

    uint8_t lx_reverse;
    uint8_t ly_reverse;
    uint8_t rx_reverse;
    uint8_t ry_reverse;

    ProcdGamepadData_t gamepad;

    Wheel_t wheels[4];
}Mcb_t;

Mcb_t MainCtrlBlk;
TskHandle_t htskCtrlMain=NULL;
static const float qtr_pi = PI/4;

void ctrlMcbInit(void)
{
    GPCtrl.vib_m0 = 0;
    GPCtrl.vib_m1 = 0;
    GPCtrl.pull_invl_ms = GAMEPAD_CHK_INVL_MS;
    GPCtrl.vib_last_ms = GAMEPAD_VIB_LAST_MS;

    PwmCtrl.tsk_period_ms = PWM_OUT_TSK_PERIOD_MS;
    PwmCtrl.out_period_us = PWM_OUT_PERIOD_US;
    for(uint32_t i=0;i<4;i++)
        PwmCtrl.out_pulse_us[i] = PWM_OUT_PULSE_MID;

    /*��Ϊҡ�����ϣ�ԭʼ���ݱ�С�����ݸ���ʹ��ϰ��Y����Ҫ����һ��*/
    MainCtrlBlk.lx_reverse = 0;
    MainCtrlBlk.ly_reverse = 1;  
    MainCtrlBlk.rx_reverse = 0;
    MainCtrlBlk.ry_reverse = 1;  

    MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].axis = WHL_AXIS_X;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].pwm_ch = 0;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].cali_mul = 128;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].cali_shift_right = 7;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].cali_offset = 0;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].out_pulse_us_min = 0;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].out_pulse_us_max = 2000;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].out_pulse_us = PWM_OUT_PULSE_MID;
 
    MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].axis = WHL_AXIS_Y;
    MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].pwm_ch = 1;
    MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].cali_mul = 128;
    MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].cali_shift_right = 7;
    MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].cali_offset = 0;
    MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].out_pulse_us_min = 0;
    MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].out_pulse_us_max = 2000;
    MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].out_pulse_us = PWM_OUT_PULSE_MID;

    MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].axis = WHL_AXIS_Y;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].pwm_ch = 2;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].cali_mul = 128;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].cali_shift_right = 7;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].cali_offset = 0;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].out_pulse_us_min = 0;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].out_pulse_us_max = 2000;
    MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].out_pulse_us = PWM_OUT_PULSE_MID;

    MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].axis = WHL_AXIS_X;
    MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].pwm_ch = 3;
    MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].cali_mul = 128;
    MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].cali_shift_right = 7;
    MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].cali_offset = 0;
    MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].out_pulse_us_min = 0;
    MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].out_pulse_us_max = 2000;
    MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].out_pulse_us = PWM_OUT_PULSE_MID;

    for(size_t i=0; i<4; i++)
    {
        int32_t cali_div = 1<<MainCtrlBlk.wheels[i].cali_shift_right;
        float mul = (float)MainCtrlBlk.wheels[i].cali_mul / (float)cali_div;
        MainCtrlBlk.wheels[i].cali_mul_f = mul;
        MainCtrlBlk.wheels[i].cali_offset_f = MainCtrlBlk.wheels[i].cali_offset;
    }
}

static inline float __joystick_proc(uint8_t raw)
{
    /*  
        ҡ��ԭʼֵ0~255����λ128����127��
        ����=0������=255������=0������=255
    */
    return ((float)raw-128.0)/128.0;


}
static inline float __pressure_proc(uint8_t raw)
{
    return ((float)raw/255.0);
}
void ctrlGamepadProc(void)
{
    const uint32_t data_len_with_joystick=3;
    if(GPDat.len >= data_len_with_joystick)
    {
        MainCtrlBlk.gamepad.LX = __joystick_proc(GPDat.joystick_LX);
        MainCtrlBlk.gamepad.LY = __joystick_proc(GPDat.joystick_LY);
        MainCtrlBlk.gamepad.RX = __joystick_proc(GPDat.joystick_RX);
        MainCtrlBlk.gamepad.RY = __joystick_proc(GPDat.joystick_RY);

        if(MainCtrlBlk.lx_reverse) 
        {
            MainCtrlBlk.gamepad.LX=0.0-MainCtrlBlk.gamepad.LX;
        }
        if(MainCtrlBlk.ly_reverse) 
        {
            MainCtrlBlk.gamepad.LY=0.0-MainCtrlBlk.gamepad.LY;
        }
        if(MainCtrlBlk.rx_reverse) 
        {
            MainCtrlBlk.gamepad.RX=0.0-MainCtrlBlk.gamepad.RX;
        }
        if(MainCtrlBlk.ry_reverse) 
        {
            MainCtrlBlk.gamepad.RY=0.0-MainCtrlBlk.gamepad.RY;
        }
        
    }

    const uint32_t data_len_with_pressure=9;
    if(GPDat.len == data_len_with_pressure)
    {
        MainCtrlBlk.gamepad.RIGHT     = __pressure_proc(GPDat.pressure_RIGHT);
        MainCtrlBlk.gamepad.LEFT      = __pressure_proc(GPDat.pressure_LEFT);
        MainCtrlBlk.gamepad.UP        = __pressure_proc(GPDat.pressure_UP);
        MainCtrlBlk.gamepad.DOWN      = __pressure_proc(GPDat.pressure_DOWN);
        MainCtrlBlk.gamepad.TRIANGLE  = __pressure_proc(GPDat.pressure_TRIANGLE);
        MainCtrlBlk.gamepad.CIRCLE    = __pressure_proc(GPDat.pressure_CIRCLE);
        MainCtrlBlk.gamepad.CROSS     = __pressure_proc(GPDat.pressure_CROSS);
        MainCtrlBlk.gamepad.SQUARE    = __pressure_proc(GPDat.pressure_SQUARE);
        MainCtrlBlk.gamepad.L1        = __pressure_proc(GPDat.pressure_L1);
        MainCtrlBlk.gamepad.R1        = __pressure_proc(GPDat.pressure_R1);
        MainCtrlBlk.gamepad.L2        = __pressure_proc(GPDat.pressure_L2);
        MainCtrlBlk.gamepad.R2        = __pressure_proc(GPDat.pressure_R2);
    }
}

static inline uint32_t __joystick_raw_2_pwm(uint8_t joystick, uint8_t reverse, 
                                            Wheel_t * wheel)
{
    int32_t data_in = (int32_t)joystick;
    int32_t data_out;

    /* �ֱ�ҡ�˵Ĳ���ֵ��0~255��127/128���е㡣
       �е�128��Ӧpwm������1500us��0��Ӧ1000us�� 255��Ӧ2000us��*/
    if(reverse)
    {
        data_out=128-data_in;
    }
    else
    {
        data_out=data_in-128;
    }
    data_out = data_out*500>>7;

    data_out = (data_out * wheel->cali_mul) >> wheel->cali_shift_right;
    data_out = data_out + wheel->cali_offset;

    data_out += 1500;

    if(data_out > wheel->out_pulse_us_max)
    {
        data_out = wheel->out_pulse_us_max;
    }    
    if(data_out < wheel->out_pulse_us_min)
    {
        data_out = wheel->out_pulse_us_min;
    }   
    return (uint32_t)data_out;
}

static void __controller_main(void)
{
    /* 
        ���ް�ť���£����ֲ�����
        ÿ��ѭ��������һ�Σ���ֹ��ť©�쵼�µ���޷�ͣ������
     */
    for(uint32_t i=0; i<4; i++)
        MainCtrlBlk.wheels[i].out_pulse_us = PWM_OUT_PULSE_MID;

    if(1==gpIsDataValid(&GPDat))
    {
        if(GPDat.L1==GP_BTN_SET || 
            GPDat.L2==GP_BTN_SET || 
            GPDat.R1==GP_BTN_SET ||
            GPDat.R2==GP_BTN_SET)
        {
            /* L1����ǰ�֣�L2������֣�R1����ǰ�֣�R2���Һ��� */
            /* ��ҡ�˿��� */
            uint8_t Y = GPDat.joystick_LY;
            
            if(GPDat.L1==GP_BTN_SET)
            {
                MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].out_pulse_us = 
                    __joystick_raw_2_pwm(Y, 
                                        MainCtrlBlk.ly_reverse, 
                                        MainCtrlBlk.wheels+WHL_IDX_FRONT_LEFT);
            }
            if(GPDat.L2==GP_BTN_SET)
            {
                MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].out_pulse_us = 
                    __joystick_raw_2_pwm(Y, 
                                        MainCtrlBlk.ly_reverse, 
                                        MainCtrlBlk.wheels+WHL_IDX_BACK_LEFT);
            }        
            if(GPDat.R1==GP_BTN_SET)
            {
                MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].out_pulse_us = 
                    __joystick_raw_2_pwm(Y, 
                                        MainCtrlBlk.ly_reverse, 
                                        MainCtrlBlk.wheels+WHL_IDX_FRONT_RIGHT);
            }        
            if(GPDat.R2==GP_BTN_SET)
            {
                MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].out_pulse_us = 
                    __joystick_raw_2_pwm(Y, 
                                        MainCtrlBlk.ly_reverse, 
                                        MainCtrlBlk.wheels+WHL_IDX_BACK_RIGHT);
            }   
        }
        else
        {   
            ctrlGamepadProc();
            

            /******************��ҡ�˿���ƽ��**************************/
            /* 
                С������ϵ�У�y���ʾǰ�� x���ʾ���ҡ�
                ҡ��ֵ������һ���������[-1.0~1.0]֮�䣨��ǰ����/���Ҹ��󣩣�
                ���ڱ�ʾС������ϵ���û��������ٶȡ�
             */
            Coord_t dot_car;
            dot_car.x = MainCtrlBlk.gamepad.LX;
            dot_car.y = MainCtrlBlk.gamepad.LY;

            // printf("%.1f,%.1f\r\n", dot_car.x, dot_car.y);

            /* 
                �Ƚ��û������ٶ���С������ϵ��ת����С��������ϵ�С�
            */
            mCart2Polar(&dot_car);

            // printf("%.1f,%.1f\r\n", dot_car.p*180.0/PI, dot_car.r);

            /*
                ����������壨p,r����
                ��ʾ��r�����ϣ������ٶ�v��С����r�����������������ٶ�v_max
                �ı�����r��Ҳ����˵��r���ֵ��1.0��
            */
            float ratio;
            if(dot_car.r>1.0) 
            {ratio=1.0;}
            else
            {ratio=dot_car.r;}

            /*
                ���û������ٶ�ת���������ķ������ϵ�С�
            */
            Coord_t dot_mecanum;
            memcpy(&dot_mecanum, &dot_car, sizeof(Coord_t));
            dot_mecanum.p -= qtr_pi;
            if(dot_mecanum.p<0.0) {dot_mecanum.p = dot_mecanum.p+PI+PI;}
            mPolar2Cart(&dot_mecanum);

            // printf("%.1f,%.1f\r\n", dot_mecanum.p*180.0/PI, dot_mecanum.r);
            // printf("%.1f,%.1f\r\n", dot_mecanum.x, dot_mecanum.y);

            if(fabs(dot_mecanum.x-0.0)<0.001)
            {
                if(dot_mecanum.y>0)
                    dot_mecanum.y=ratio;
                else
                    dot_mecanum.y=0.0-ratio;

                dot_mecanum.x=0.0;
            }
            else if(fabs(dot_mecanum.y-0.0)<0.001)
            {
                if(dot_mecanum.x>0)
                    dot_mecanum.x=ratio;
                else
                    dot_mecanum.x=0.0-ratio;

                dot_mecanum.y=0.0;
            }
            else
            {
                float _tan = dot_mecanum.y/dot_mecanum.x;
                float _tan_abs = fabsf(_tan);

                // printf("%.1f,%.1f\r\n", ratio, _tan_abs);

                if(_tan_abs>1)
                {
                    if(dot_mecanum.y>0.0)
                    {dot_mecanum.y=ratio;}    
                    else
                    {dot_mecanum.y=0.0-ratio;}    

                    if(dot_mecanum.x>0.0)
                    {dot_mecanum.x=ratio/_tan_abs;}
                    else
                    {dot_mecanum.x=0.0-ratio/_tan_abs;
                    }
                    
                }
                else
                {
                    if(dot_mecanum.x>0.0)
                    {dot_mecanum.x=ratio;}    
                    else
                    {dot_mecanum.x=0.0-ratio;}    

                    if(dot_mecanum.y>0.0)
                    {dot_mecanum.y=ratio*_tan_abs;}
                    else
                    {dot_mecanum.y=0.0-ratio*_tan_abs;}
                }
            }

            // printf("%.1f,%.1f\r\n", dot_mecanum.x, dot_mecanum.y);


            for(uint32_t i=0; i<4; i++)
            {
                if(MainCtrlBlk.wheels[i].axis == WHL_AXIS_X)
                {
                    MainCtrlBlk.wheels[i].out_percentage = dot_mecanum.x;
                }
                else if(MainCtrlBlk.wheels[i].axis == WHL_AXIS_Y)
                {
                    MainCtrlBlk.wheels[i].out_percentage = dot_mecanum.y;
                }
                else
                {
                    printf("%s, line%u: This is not suppose to happen.\r\n", \
                        __func__, __LINE__);
                }
            }

            /******************��ҡ�˿�����̬**************************/
            /*����ԭ����ת���������ת���Ҳ��ַ�ת*/
            /*����ԭ����ת������ַ�ת���Ҳ�����ת*/

            if(MainCtrlBlk.rx_reverse)
            {
                MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].out_percentage -= MainCtrlBlk.gamepad.RX;
                MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].out_percentage -= MainCtrlBlk.gamepad.RX;
                MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].out_percentage += MainCtrlBlk.gamepad.RX;
                MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].out_percentage += MainCtrlBlk.gamepad.RX;
            } 
            else
            {
                MainCtrlBlk.wheels[WHL_IDX_FRONT_LEFT].out_percentage += MainCtrlBlk.gamepad.RX;
                MainCtrlBlk.wheels[WHL_IDX_BACK_LEFT].out_percentage += MainCtrlBlk.gamepad.RX;
                MainCtrlBlk.wheels[WHL_IDX_FRONT_RIGHT].out_percentage -= MainCtrlBlk.gamepad.RX;
                MainCtrlBlk.wheels[WHL_IDX_BACK_RIGHT].out_percentage -= MainCtrlBlk.gamepad.RX;
            }
             
            /* ��һ��ת��Ϊpwm���ֵ */
            for(uint32_t i=0; i<4; i++)
            {
                /*���ϳ�����̬��ת�󣬷�ֹ���*/
                if(MainCtrlBlk.wheels[i].out_percentage>1.0)
                {MainCtrlBlk.wheels[i].out_percentage=1.0;}
                if(MainCtrlBlk.wheels[i].out_percentage<-1.0)
                {MainCtrlBlk.wheels[i].out_percentage=-1.0;}
                
                /*���У׼*/
                MainCtrlBlk.wheels[i].out_percentage *= 
                                        MainCtrlBlk.wheels[i].cali_mul_f;
                MainCtrlBlk.wheels[i].out_percentage += 
                                        MainCtrlBlk.wheels[i].cali_offset_f;

                /* ��һ��ת��Ϊpwm���ֵ */
                MainCtrlBlk.wheels[i].out_pulse_us = 
                    (int32_t)(500.0*MainCtrlBlk.wheels[i].out_percentage)+1500;

                /*pwm�����С����*/
                if(MainCtrlBlk.wheels[i].out_pulse_us > MainCtrlBlk.wheels[i].out_pulse_us_max)
                {
                    MainCtrlBlk.wheels[i].out_pulse_us = MainCtrlBlk.wheels[i].out_pulse_us_max;
                }
                if(MainCtrlBlk.wheels[i].out_pulse_us < MainCtrlBlk.wheels[i].out_pulse_us_min)
                {
                    MainCtrlBlk.wheels[i].out_pulse_us = MainCtrlBlk.wheels[i].out_pulse_us_min;
                }
            }

        }
    }

    /* ���ݳ��ֺ�pwm���ͨ����ӳ���ϵ���� */
    for(uint32_t i=0; i<4; i++)
        PwmCtrl.out_pulse_us[MainCtrlBlk.wheels[i].pwm_ch] = 
                                        MainCtrlBlk.wheels[i].out_pulse_us;
}

void ctrlTskInit(void)
{
    htskCtrlMain = tskAdd("Controller", __controller_main, 0, 0);
    if(!htskCtrlMain)
    {
        printf("%s, line%u: Fail to create controller main task!!!\r\n", \
        __func__, __LINE__);
    }
}
