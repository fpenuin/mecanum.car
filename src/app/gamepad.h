/*
 * gamepad.h
 *
 *  Created on: 2020Äê4ÔÂ10ÈÕ
 *      Author: fpenguin
 */

#ifndef APP_GAMEPAD_H_
#define APP_GAMEPAD_H_

#include <stdint.h>
#include <stddef.h>

#include "../frsvd/inc/tsk.h"

#define GP_BTN_SET      (0)
#define GP_BTN_RESET    (1)

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)
typedef struct __GamePadData_t{
        uint32_t rsvd0:8;

        uint32_t len:4;
        uint32_t mode:4;

        uint32_t rsvd1:8;

        uint32_t SELECT:1;
        uint32_t L3:1;
        uint32_t R3:1;
        uint32_t START:1;
        uint32_t UP:1;
        uint32_t RIGHT:1;
        uint32_t DOWN:1;
        uint32_t LEFT:1;
        
        uint32_t L2:1;
        uint32_t R2:1;
        uint32_t L1:1;
        uint32_t R1:1;
        uint32_t TRIANGLE:1;
        uint32_t CIRCLE:1;
        uint32_t CROSS:1;
        uint32_t SQUARE:1;

        uint8_t joystick_RX;
        uint8_t joystick_RY;
        uint8_t joystick_LX;
        uint8_t joystick_LY;

        uint8_t pressure_RIGHT;
        uint8_t pressure_LEFT;
        uint8_t pressure_UP;
        uint8_t pressure_DOWN;
        uint8_t pressure_TRIANGLE;
        uint8_t pressure_CIRCLE;
        uint8_t pressure_CROSS;
        uint8_t pressure_SQUARE;
        uint8_t pressure_L1;
        uint8_t pressure_R1;
        uint8_t pressure_L2;
        uint8_t pressure_R2;

}GamePadData_t;
#pragma pack(pop)

typedef struct  __GamePadCtrl_t
{
    uint8_t vib_m0;
    uint8_t vib_m1;
    uint16_t vib_last_ms;

    uint32_t pull_invl_ms;

}GamePadCtrl_t;


extern GamePadData_t GPDat;
extern GamePadCtrl_t GPCtrl;
extern TskHandle_t htskGamePadChk;
extern TskHandle_t htskGamePadPrint;

void gpTskInit(void);

uint32_t gpIsDataValid(GamePadData_t * data);
void gpPrint(GamePadData_t * data, uint32_t joystick_and_pressure);

void gpSpiInit(uint16_t SPI_BaudRatePrescaler);
void gpSpiXfer(uint8_t * data_out, uint8_t * data_in, uint16_t data_len);

void gpPullChk(void);
void gpPull(uint8_t vibr0, uint8_t vibr1, GamePadData_t * gpdat);
void gpConfig(uint32_t enter);
void gpMode(uint32_t mode, uint32_t lock);
void gpVib(uint32_t motor0, uint32_t motor1);
void gpPressure(uint32_t arrow, uint32_t color, uint32_t trig);


int CMD_GpChkInvl(char * arg);
int CMD_GpVib(char * arg);
int CMD_GpPrint(char * arg);

int CMD_GpSpiInit(char * arg);
int CMD_GpSpiXfer(char * arg);
int CMD_GpSpiPullChk(char * arg);
int CMD_GpSpiPull(char * arg);
int CMD_GpSpiConfig(char * arg);
int CMD_GpSpiMode(char * arg);
int CMD_GpSpiVib(char * arg);
int CMD_GpSpiPress(char * arg);
int CMD_GpTest(char * arg);

int regGamepadCmds(void);

#endif /* APP_GAMEPAD_H_ */
