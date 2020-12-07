/*
 * controller.c
 *
 *  Created on: 2020年10月31日
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


/* 控制数据结构体中各个数据的取值 **********************************/

/* 正转时，往右前方移动的轮子为WHEELS_AXIS_X；
    正转时，往左前方移动的轮值为WHEELS_AXIS_Y。 */
#define WHL_AXIS_X (0)
#define WHL_AXIS_Y (1)

/* 数组下标与轮子的对应关系 */
#define WHL_IDX_FRONT_LEFT   (0)
#define WHL_IDX_FRONT_RIGHT  (1)
#define WHL_IDX_BACK_LEFT    (2)
#define WHL_IDX_BACK_RIGHT   (3)


/* 在这里调整各种参数***********************************************/
#define PWM_OUT_TSK_PERIOD_MS   (10)
#define PWM_OUT_PULSE_MID       (1500)
#define PWM_OUT_PERIOD_US       (20000)


#define GAMEPAD_CHK_INVL_MS (10)
#define GAMEPAD_VIB_LAST_MS (100)

typedef struct __Wheel_t{

    uint32_t axis;
    uint32_t pwm_ch;

    /*
        以下三个参数用于校准电机，保证各个电机转速相同，小测跑直线。

        data_calibrated =
            (data_raw * cali_mul) >> cali_shift_right + cali_offset

        使用线性校准： y=ax+b。 为提高运算效率，系数a被拆分为 cali_mul 和
        cali_shift_right。一般cali_shift_right取7，即a=cali_mul/128。
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

    /*因为摇杆往上，原始数据变小，根据个人使用习惯Y轴需要反向一下*/
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
        摇杆原始值0~255，中位128或者127。
        最上=0，最下=255，最左=0，最右=255
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

    /* 手柄摇杆的采样值在0~255，127/128是中点。
       中点128对应pwm的脉宽1500us，0对应1000us， 255对应2000us。*/
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
        若无按钮按下，保持不动。
        每个循环都设置一次，防止按钮漏检导致电机无法停下来。
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
            /* L1：左前轮，L2：左后轮，R1：右前轮，R2：右后轮 */
            /* 左摇杆控制 */
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
            

            /******************左摇杆控制平移**************************/
            /* 
                小车坐标系中，y轴表示前后， x轴表示左右。
                摇杆值经过归一化处理后，在[-1.0~1.0]之间（正前负后/正右负左），
                用于表示小车坐标系中用户期望的速度。
             */
            Coord_t dot_car;
            dot_car.x = MainCtrlBlk.gamepad.LX;
            dot_car.y = MainCtrlBlk.gamepad.LY;

            // printf("%.1f,%.1f\r\n", dot_car.x, dot_car.y);

            /* 
                先将用户期望速度在小车坐标系中转换到小车极坐标系中。
            */
            mCart2Polar(&dot_car);

            // printf("%.1f,%.1f\r\n", dot_car.p*180.0/PI, dot_car.r);

            /*
                极坐标的意义（p,r）：
                表示在r方向上，期望速度v与小车在r方向上能输出的最大速度v_max
                的比例是r。也就是说，r最大值是1.0。
            */
            float ratio;
            if(dot_car.r>1.0) 
            {ratio=1.0;}
            else
            {ratio=dot_car.r;}

            /*
                将用户期望速度转换到麦克纳姆极坐标系中。
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

            /******************右摇杆控制姿态**************************/
            /*车身原地右转：左侧轮正转，右侧轮反转*/
            /*车身原地左转：左侧轮反转，右侧轮正转*/

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
             
            /* 归一化转换为pwm输出值 */
            for(uint32_t i=0; i<4; i++)
            {
                /*加上车身姿态旋转后，防止溢出*/
                if(MainCtrlBlk.wheels[i].out_percentage>1.0)
                {MainCtrlBlk.wheels[i].out_percentage=1.0;}
                if(MainCtrlBlk.wheels[i].out_percentage<-1.0)
                {MainCtrlBlk.wheels[i].out_percentage=-1.0;}
                
                /*电机校准*/
                MainCtrlBlk.wheels[i].out_percentage *= 
                                        MainCtrlBlk.wheels[i].cali_mul_f;
                MainCtrlBlk.wheels[i].out_percentage += 
                                        MainCtrlBlk.wheels[i].cali_offset_f;

                /* 归一化转换为pwm输出值 */
                MainCtrlBlk.wheels[i].out_pulse_us = 
                    (int32_t)(500.0*MainCtrlBlk.wheels[i].out_percentage)+1500;

                /*pwm最大最小限制*/
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

    /* 根据车轮和pwm输出通道的映射关系设置 */
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
