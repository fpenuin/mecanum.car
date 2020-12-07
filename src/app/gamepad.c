/*
 * gamepad.c
 *
 *  Created on: 2020��4��10��
 *      Author: fpenguin
 */


#include "gamepad.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "stm32f4xx_hal.h"

#include "../frsvd/inc/cmd.h"
#include "../stm32f4.frsvd/dwt_cycle.h"



/*������������ĸ��ֽڶ���*/
#define DS2_CMD_BYTE1_HDR     (0x01)

#define DS2_CMD_BYTE2_POLLCHK (0x41)
#define DS2_CMD_BYTE2_POLL    (0x42)
#define DS2_CMD_BYTE2_ESC     (0x43)
#define DS2_CMD_BYTE2_MODE    (0x44)
#define DS2_CMD_BYTE2_VIBR    (0x4D)
#define DS2_CMD_BYTE2_BUTTONS (0x4F)

#define DS2_CMD_BYTE3_DUMMY   (0x00)

#define DS2_CMD_BYTE4_CFG_ENTER   (0x01)
#define DS2_CMD_BYTE4_CFG_EXIT    (0x00)

#define DS2_CMD_BYTE4_MODE_ANALOG        (0x01)
#define DS2_CMD_BYTE4_MODE_DIGITAL       (0x00)
#define DS2_CMD_BYTE5_MODE_ANALOG_LOCK   (0x03)
#define DS2_CMD_BYTE5_MODE_ANALOG_NOLOCK (0xee) 

#define DS2_CMD_BYTE4_VIBR_EN0   (0x00)
#define DS2_CMD_BYTE4_VIBR_DIS   (0xff)
#define DS2_CMD_BYTE5_VIBR_EN1   (0x01)
#define DS2_CMD_BYTE5_VIBR_DIS   (0xff)

#define DS2_CMD_BYTEx_DUMMY     (0x00)

/*�ֱ���������*/
#define DS2_ARG_MODE_ANALOG     (1)
#define DS2_ARG_MODE_DIGITAL    (0)
#define DS2_ARG_MODE_LOCK       (1)
#define DS2_ARG_MODE_NOLOCK     (0)

#define DS2_ARG_VIBR_ON     (1)
#define DS2_ARG_VIBR_OFF    (0)

#define DS2_ARG_CONFIG_ON   (1)
#define DS2_ARG_CONFIG_OFF  (0)

#define DS2_ARG_PRESSURE_ON     (1)
#define DS2_ARG_PRESSURE_OFF    (0)

/*֡ͷ����*/
#define DS2_CMD_HDR_LEN (3)

/*
��������֡����Ҫ�̶�����
��ʵ֤��ģʽ�л�����ʹ������ȱ���̶�
�����ܻ��������ֵֹ����⣺
1. ��ģʽ�л�����ȵ���2ʱ��digital�л���analog�᲻�ɹ���
2. ����ʹ�ܵ���6ʱ��pull�����޷�ʹС����񶯡�
������˳�����ģʽ������ȣ��������ֱ��ظ����غɳ�����ȷ�������û���⡣
*/
#define DS2_CMD_MODE_LEN (6)
#define DS2_CMD_VIBR_LEN (2)
#define DS2_CMD_BUTTONS_LEN (6)
#define DS2_CMD_PULLCHK_LEN (6)


/*ʶ��֡ͷ�е�ģʽ*/
#define DS2_HDR_MODE_DIGITAL    (0x4)
#define DS2_HDR_MODE_ANALOG     (0x7)
#define DS2_HDR_MODE_CONFIG     (0xF)

/*�ֽڼ���ʱ���Լ�Ƭѡ����ʱ*/
#define BYTE_DELAY_US   (8)     /*��С6us*/
#define CS_STT_DELAY_US (6)    /*��С4us*/
#define CS_STP_DELAY_US (7)    /*��С5us*/

/*�ֱ��ظ����������֡��*/
#define MAX_SPI_BUFF    (21)

/*ʹ��GPIO��Ƭѡ������NSS*/
#define CS_HIGH()   do {\
                        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); \
                        cycDelayUs(CS_STP_DELAY_US); \
                    }while(0)
#define CS_LOW()    do {\
                        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); \
                        cycDelayUs(CS_STT_DELAY_US); \
                    }while(0)

/*SPI��ʼ�����ֱ���������Ӱ�죬�ݲ��������������ʱ����ֱ���������*/
#define SPI_INIT_DELAY_MS (3000)

#define DEBUG_PRINT (0)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-label"

static SPI_HandleTypeDef __gamepad_spi;

GamePadData_t GPDat;
GamePadCtrl_t GPCtrl;
TskHandle_t htskGamePadChk=NULL;
TskHandle_t htskGamePadPrint=NULL;

static void __gamepad_main(void);
static void __gamepad_main(void)
{
    static uint32_t stt=0;
    static uint8_t m0=0, m1=0;

    if(GPCtrl.vib_m0 || GPCtrl.vib_m1)
    {
        m0=GPCtrl.vib_m0;
        m1=GPCtrl.vib_m1;

        GPCtrl.vib_m0=0;
        GPCtrl.vib_m1=0;

        stt=HAL_GetTick(); /* Ĭ����1 tick = 1 ms */
    }

    uint32_t now=HAL_GetTick();
    if(now-stt > GPCtrl.vib_last_ms)
    {
        m0=m1=0;
    }

    gpPull(m0, m1, &GPDat);
}

void gpTskInit(void)
{
    memset(&GPDat, 0, sizeof(GPDat));

    /* 50MHz / 128 = 390.625kHz */
    gpSpiInit(SPI_BAUDRATEPRESCALER_128);
    cycDelayMs(SPI_INIT_DELAY_MS);

    gpConfig(DS2_ARG_CONFIG_ON);
    gpMode(DS2_ARG_MODE_ANALOG, DS2_ARG_MODE_NOLOCK);
    gpVib(DS2_ARG_VIBR_ON, DS2_ARG_VIBR_ON);
    gpPressure(DS2_ARG_PRESSURE_ON, DS2_ARG_PRESSURE_ON, DS2_ARG_PRESSURE_ON);
    gpConfig(DS2_ARG_CONFIG_OFF);
    
    htskGamePadChk = tskAdd("GamePadChk", __gamepad_main, 0,0);
    if(!htskGamePadChk)
    {
        printf("%s, line%u: Fail to create gamepad main task!!!\r\n", \
        __func__, __LINE__);
    }

    tskPeriod(htskGamePadChk, GPCtrl.pull_invl_ms);
}

uint32_t gpIsDataValid(GamePadData_t * data)
{
    uint32_t data_valid = 0;
    switch(data->mode)
    {
        case DS2_HDR_MODE_ANALOG:
            if( (data->len==3) && \
                (data->rsvd0==0xFF) && \
                (data->rsvd1==0x00) )
            {data_valid=1;}
            else if( (data->len==9) && \
                (data->rsvd0==0xFF) && \
                (data->rsvd1==0x5A) )
            {data_valid=1;}
            else
            {data_valid=0;}
            break; 
        case DS2_HDR_MODE_DIGITAL:
            if( (data->len==1) && \
                (data->rsvd0==0xFF) && \
                (data->rsvd1==0x00) )
            {data_valid=1;}
            else
            {data_valid=0;}
            break;
        case DS2_HDR_MODE_CONFIG:
            data_valid=0;
            break;
        default:
            data_valid=0;
    }
    return data_valid;
}
void gpPrint(GamePadData_t * data, uint32_t joystick_and_pressure)
{
    printf("Mode: ");
    switch(data->mode)
    {
        case DS2_HDR_MODE_ANALOG:
            printf("Analog\r\n");
            break;
        case DS2_HDR_MODE_DIGITAL:
            printf("Digital\r\n");
            break;
        case DS2_HDR_MODE_CONFIG:
            printf("Configuration\r\n");
            break;
        default:
            printf("Illegal value, mode=0x%x.\r\n", data->mode);
    }

    printf("Data len: %u Bytes.\r\n", data->len * 2);
    printf("/*******************************************"
        "*****************************************/\r\n");
    printf("%-2s | %-2s | %-2s | %-2s || %-2s | %-2s | %-2s | %-2s || " \
            "%-2s | %-2s | %-2s | %-2s || %-2s | %-2s | %-2s | %-2s \r\n",
            "SL","ST","L3","R3","L1","L2","R1","R2",\
            "U","D","L","R","/\\","X","[]","O");
    printf("%-2u | %-2u | %-2u | %-2u || %-2u | %-2u | %-2u | %-2u || " \
            "%-2u | %-2u | %-2u | %-2u || %-2u | %-2u | %-2u | %-2u \r\n",
            data->SELECT, data->START, data->L3, data->R3, \
            data->L1, data->L2, data->R1, data->R2, \
            data->UP, data->DOWN, data->LEFT, data->RIGHT, \
            data->TRIANGLE, data->CROSS, data->SQUARE, data->CIRCLE);
    if(joystick_and_pressure>0)
    {
        printf("/*******************************************"
                "*****************************************/\r\n");
        printf("%-4s | %-4s | %-4s | %-4s \r\n",
                "LX", "LY", "RX", "RY");
        printf("%-4u | %-4u | %-4u | %-4u \r\n", \
                data->joystick_LX, data->joystick_LY, \
                data->joystick_RX, data->joystick_RY);
    }

    if(joystick_and_pressure>1)
    {
        printf("/*******************************************"
        "*****************************************/\r\n");
        printf("%-4s | %-4s | %-4s | %-4s || " \
                "%-4s | %-4s | %-4s | %-4s || " \
                "%-4s | %-4s | %-4s | %-4s \r\n",
                "L1","L2","R1","R2", 
                "U","D","L","R", 
                "/\\","X","[]","O");
        printf("%-4u | %-4u | %-4u | %-4u || " \
                "%-4u | %-4u | %-4u | %-4u || "\
                "%-4u | %-4u | %-4u | %-4u\r\n", \
                data->pressure_L1, data->pressure_L2, 
                data->pressure_R1, data->pressure_R2,
                data->pressure_UP, data->pressure_DOWN, 
                data->pressure_LEFT, data->pressure_RIGHT, 
                data->pressure_TRIANGLE, data->pressure_CROSS, 
                data->pressure_SQUARE, data->pressure_CIRCLE);
    }
    printf("/*******************************************"
        "*****************************************/\r\n");
}

void gpSpiInit(uint16_t SPI_BaudRatePrescaler)
{
    __gamepad_spi.Instance                  = SPI2;
    __gamepad_spi.Init.BaudRatePrescaler    = SPI_BaudRatePrescaler;

    __gamepad_spi.Init.Mode                 = SPI_MODE_MASTER;
    __gamepad_spi.Init.Direction            = SPI_DIRECTION_2LINES;
    __gamepad_spi.Init.DataSize             = SPI_DATASIZE_8BIT;
    __gamepad_spi.Init.FirstBit             = SPI_FIRSTBIT_LSB;
    __gamepad_spi.Init.CLKPolarity          = SPI_POLARITY_HIGH;
    __gamepad_spi.Init.CLKPhase             = SPI_PHASE_2EDGE;
    __gamepad_spi.Init.NSS                  = SPI_NSS_HARD_OUTPUT;
    
    __gamepad_spi.Init.TIMode               = SPI_TIMODE_DISABLED;
    __gamepad_spi.Init.CRCCalculation       = SPI_CRCCALCULATION_DISABLED;
    HAL_SPI_Init(&__gamepad_spi);

    CS_HIGH();
}
void gpSpiXfer(uint8_t * data_out, uint8_t * data_in, uint16_t data_len)
{
    assert(data_out);
    assert(data_in);

    for(size_t i=0;i<data_len;i++)
    {
        for(HAL_StatusTypeDef state=HAL_BUSY; state!=HAL_OK; )
        { state = HAL_SPI_TransmitReceive(&__gamepad_spi, data_out+i, data_in+i, 1, 65535); }

        cycDelayUs(BYTE_DELAY_US);  //ÿ���ֽڲ�����ʱ
    }

    return;
}


void gpPullChk(void)
{
    uint8_t data_out[MAX_SPI_BUFF], data_in[MAX_SPI_BUFF];
    uint8_t playload_len, hdr_len=DS2_CMD_HDR_LEN;
    
    memset(data_out, 0x5a, MAX_SPI_BUFF);

    /*Ƭѡ����*/
    CS_LOW(); 

    /*����֡ͷ*/
    data_out[0] = DS2_CMD_BYTE1_HDR;
    data_out[1] = DS2_CMD_BYTE2_POLLCHK;
    data_out[2] = DS2_CMD_BYTE3_DUMMY;
    gpSpiXfer(data_out, data_in, hdr_len);

    /*�����غ����ݳ���*/
    playload_len = DS2_CMD_PULLCHK_LEN;

    /*������������*/
    gpSpiXfer(data_out+hdr_len, data_in+hdr_len, playload_len);

    /*Ƭѡ����*/
    CS_HIGH();

// #if DEBUG_PRINT
    printf("%s data_out:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_out[i]);
    }
    printf("\r\n");
    printf("%s data_in:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_in[i]);
    }
    printf("\r\n");
// #endif
}
void gpPull(uint8_t vibr0, uint8_t vibr1, GamePadData_t * gpdat)
{
    /*
        �Ա�����ֻ�ֱ���vibr1�����65��ʼ�𶯣�
        �ٵ;Ͳ����ˣ�65��255�𶯷�������̫��
    */
    
    assert(gpdat);

    uint8_t data_out[MAX_SPI_BUFF];
    uint8_t * data_in=(uint8_t *)(gpdat);
    uint8_t playload_len, hdr_len=DS2_CMD_HDR_LEN;
    const uint8_t min_len=2;
    
    memset(data_out, 0, MAX_SPI_BUFF);

    /*Ƭѡ����*/
    CS_LOW();

    /*����֡ͷ*/
    data_out[0] = DS2_CMD_BYTE1_HDR;
    data_out[1] = DS2_CMD_BYTE2_POLL;
    data_out[2] = DS2_CMD_BYTE3_DUMMY;
    gpSpiXfer(data_out, data_in, hdr_len);

    /*�����غ����ݳ���*/
    playload_len = ((data_in[1]&0xf) << 1);
    if( playload_len<min_len || playload_len>(sizeof(GamePadData_t)-3) )
    {
        printf("%s, line %u: error on palyload_len.\r\n", \
                __func__, __LINE__);
        printf("header: ");
        for(size_t i=0; i<hdr_len; i++)
           {printf("0x%02X ", data_in[i]);}
        printf("\r\n");
        playload_len=0;
        goto exit;
    }

    /*������������*/
    data_out[3]=vibr0;
    data_out[4]=vibr1;
    gpSpiXfer(data_out+hdr_len, data_in+hdr_len, playload_len);

exit:
    /*Ƭѡ����*/
    CS_HIGH();

#if DEBUG_PRINT
    printf("%s data_out:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_out[i]);
    }
    printf("\r\n");
    printf("%s data_in:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_in[i]);
    }
    printf("\r\n");
#endif    
}
void gpConfig(uint32_t enter)
{
    uint8_t data_out[MAX_SPI_BUFF], data_in[MAX_SPI_BUFF];
    uint8_t playload_len, hdr_len=DS2_CMD_HDR_LEN;
    const uint8_t min_len=1;

    memset(data_out, 0, MAX_SPI_BUFF);

    /*Ƭѡ����*/
    CS_LOW(); 

    /*����֡ͷ*/
    data_out[0] = DS2_CMD_BYTE1_HDR;
    data_out[1] = DS2_CMD_BYTE2_ESC;
    data_out[2] = DS2_CMD_BYTE3_DUMMY;
    gpSpiXfer(data_out, data_in, hdr_len);

    /*�����غ����ݳ���*/
    playload_len = ((data_in[1]&0xf) << 1);
    if(playload_len<min_len || playload_len>MAX_SPI_BUFF)
    {
        printf("%s, line %u: error on palyload_len.\r\n", \
                __func__, __LINE__);
        printf("header: ");
        for(size_t i=0; i<hdr_len; i++)
           {printf("0x%02X ", data_in[i]);}
        printf("\r\n");
        playload_len=0;
        goto exit;
    }

    /*������������*/
    if(enter)
    {
        data_out[3] = DS2_CMD_BYTE4_CFG_ENTER;
    }    
    else
    {
        data_out[3] = DS2_CMD_BYTE4_CFG_EXIT;
    }
    
    gpSpiXfer(data_out+hdr_len, data_in+hdr_len, playload_len);

exit:
    /*Ƭѡ����*/
    CS_HIGH();

#if DEBUG_PRINT
    printf("%s data_out:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_out[i]);
    }
    printf("\r\n");
    printf("%s data_in:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_in[i]);
    }
    printf("\r\n");
#endif
}
void gpMode(uint32_t mode, uint32_t lock)
{
    uint8_t data_out[MAX_SPI_BUFF], data_in[MAX_SPI_BUFF];
    uint8_t playload_len, hdr_len=DS2_CMD_HDR_LEN;
    const uint8_t min_len=2;
    
    memset(data_out, 0, MAX_SPI_BUFF);

    /*Ƭѡ����*/
    CS_LOW(); 

    /*����֡ͷ*/
    data_out[0] = DS2_CMD_BYTE1_HDR;
    data_out[1] = DS2_CMD_BYTE2_MODE;
    data_out[2] = DS2_CMD_BYTE3_DUMMY;
    gpSpiXfer(data_out, data_in, hdr_len);

    /*ģʽ�л�����ȱ���̶�*/
    playload_len = DS2_CMD_MODE_LEN;

    /*������������*/
    if(mode)
    {
        data_out[3] = DS2_CMD_BYTE4_MODE_ANALOG;
        if(lock)
            {data_out[4] = DS2_CMD_BYTE5_MODE_ANALOG_LOCK;}
        else
            {data_out[4] = DS2_CMD_BYTE5_MODE_ANALOG_NOLOCK;}
    }    
    else
    {
        data_out[3] = DS2_CMD_BYTE4_MODE_DIGITAL;
        if(lock)
            {data_out[4] = DS2_CMD_BYTE5_MODE_ANALOG_LOCK;}
        else
            {data_out[4] = DS2_CMD_BYTE5_MODE_ANALOG_NOLOCK;}
    }
   
    gpSpiXfer(data_out+hdr_len, data_in+hdr_len, playload_len);

exit:
    /*Ƭѡ����*/
    CS_HIGH();

#if DEBUG_PRINT
    printf("%s data_out:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_out[i]);
    }
    printf("\r\n");
    printf("%s data_in:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_in[i]);
    }
    printf("\r\n");
#endif

}
void gpVib(uint32_t motor0, uint32_t motor1)
{
    uint8_t data_out[MAX_SPI_BUFF], data_in[MAX_SPI_BUFF];
    uint8_t playload_len, hdr_len=DS2_CMD_HDR_LEN;
    const uint8_t min_len=2;
    
    memset(data_out, 0, MAX_SPI_BUFF);

    /*Ƭѡ����*/
    CS_LOW(); 

    /*����֡ͷ*/
    data_out[0] = DS2_CMD_BYTE1_HDR;
    data_out[1] = DS2_CMD_BYTE2_VIBR;
    data_out[2] = DS2_CMD_BYTE3_DUMMY;
    gpSpiXfer(data_out, data_in, hdr_len);

    /*������ȱ���̶�*/
    playload_len = DS2_CMD_VIBR_LEN;

    /*������������*/
    if(motor0)
    {
        data_out[3] = DS2_CMD_BYTE4_VIBR_EN0;
        if(motor1)
            {data_out[4] = DS2_CMD_BYTE5_VIBR_EN1;}
        else
            {data_out[4] = DS2_CMD_BYTE5_VIBR_DIS;}
    }    
    else
    {
        data_out[3] = DS2_CMD_BYTE4_VIBR_DIS;
        if(motor1)
            {data_out[4] = DS2_CMD_BYTE5_VIBR_EN1;}
        else
            {data_out[4] = DS2_CMD_BYTE5_VIBR_DIS;}
    }
    
    gpSpiXfer(data_out+hdr_len, data_in+hdr_len, playload_len);

exit:
    /*Ƭѡ����*/
    CS_HIGH();

#if DEBUG_PRINT
    printf("%s data_out:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_out[i]);
    }
    printf("\r\n");
    printf("%s data_in:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_in[i]);
    }
    printf("\r\n");
#endif
}
void gpPressure(uint32_t arrow, uint32_t color, uint32_t trig)
{
    /*������������ø���������ѹ��ֵ�Ƿ���ڱ����С�
      ��������ѹ��ֵ��˳���ǹ̶��ģ�����û��ʹ�ܷ������ѹ��ֵ��
      ����ʹ������ɫ�����Ͱ��������ѹ��ֵ����ô�������ѹ��ֵ��
      ��Ϊ0������ֱ�Ӳ������������ڱ�����Ԥ�������ѹ��ֵ������
      λ�á�
      ʹ��gpPullChk���0x41�������ܲ�ѯ����Щ����������ѹ��ֵ
      ��Ϣ��gpPullchk���ֱܷ����analog����digitalģʽ��annalog
      ģʽ��0x41������ص���Ϣ��0xff 0xff 0x03����digitalģʽ��
      ����Ϣ��0x00 0x00 0x00��
    */
    uint8_t data_out[MAX_SPI_BUFF], data_in[MAX_SPI_BUFF];
    uint8_t playload_len, hdr_len=DS2_CMD_HDR_LEN;
        
    memset(data_out, 0, MAX_SPI_BUFF);

    /*Ƭѡ����*/
    CS_LOW(); 

    /*����֡ͷ*/
    data_out[0] = DS2_CMD_BYTE1_HDR;
    data_out[1] = DS2_CMD_BYTE2_BUTTONS;
    data_out[2] = DS2_CMD_BYTE3_DUMMY;
    gpSpiXfer(data_out, data_in, hdr_len);

#if 1
    /*���ô���ѹ����Ϣ����ȱ���̶�*/
    playload_len = DS2_CMD_BUTTONS_LEN;
#else
    const uint8_t min_len=3;
    playload_len = ((data_in[1]&0xf) << 1);
    if(playload_len<min_len || playload_len>MAX_SPI_BUFF)
    {
        printf("%s, line %u: error on palyload_len.\r\n", \
                __func__, __LINE__);
        printf("header: ");
        for(size_t i=0; i<hdr_len; i++)
           {printf("0x%02X ", data_in[i]);}
        printf("\r\n");
        playload_len=0;
        goto exit;
    }
#endif
    
    /*������������*/
    data_out[3] = 0b00111111;
    if(arrow)
    {
        data_out[3] |= 0b11000000;
        data_out[4] |= 0b00000011;
    }
    if(color)
    {
        data_out[4] |= 0b00111100;
    }
    if(trig)
    {
        data_out[4] |= 0b11000000;
        data_out[5] |= 0b00000011;
    }
    
    gpSpiXfer(data_out+hdr_len, data_in+hdr_len, playload_len);

exit:
    /*Ƭѡ����*/
    CS_HIGH();

#if DEBUG_PRINT
    printf("%s data_out:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_out[i]);
    }
    printf("\r\n");
    printf("%s data_in:", __func__);
    for(size_t i=0; i<hdr_len+playload_len; i++)
    {
        printf("0x%02X ", data_in[i]);
    }
    printf("\r\n");
#endif
}



int CMD_GpChkInvl(char * arg)
{
    uint32_t invl_ms;
    int r = cmdGetArg(arg, &invl_ms);
    if(r!=1) return CMD_ERRARG;

    GPCtrl.pull_invl_ms = invl_ms;
    if(htskGamePadChk)
    {
        tskPeriod(htskGamePadChk, GPCtrl.pull_invl_ms);
    }
    else
    {
        printf("%s, line %u: Gamepad main task was invalid.\r\n", \
                __func__, __LINE__);
    }
    

    return CMD_SUCCESS; 
}
int CMD_GpVib(char * arg)
{
    uint32_t motor0, motor1, last_ms;
    int r = cmdGetArg(arg, &motor0, &motor1, &last_ms);
    if(r!=3) return CMD_ERRARG;

    GPCtrl.vib_m0 = (uint8_t)motor0;
    GPCtrl.vib_m1 = (uint8_t)motor1;
    GPCtrl.vib_last_ms = (uint16_t)last_ms;

    return CMD_SUCCESS; 
}
int CMD_GpPrint(char * arg)
{
    uint32_t invl_ms, joystick_and_pressure;
    int r=cmdGetArg(arg, &invl_ms, &joystick_and_pressure);
    if(r!=2) return CMD_ERRARG;

    if(invl_ms)
    {   
        if(htskGamePadPrint)
        {
            tskRemove(htskGamePadPrint);
            htskGamePadPrint=NULL;
        }

        htskGamePadPrint = tskAdd( "GamePadPrint", \
                                    gpPrint, \
                                    (uint32_t)(&GPDat), \
                                    joystick_and_pressure );
        if(htskGamePadPrint)
        {
            tskPeriod(htskGamePadPrint, invl_ms);
        }
        else
        {
            printf("%s, line%u: Fail to create print task!!!\r\n", \
            __func__, __LINE__);
        }
    }
    else
    {
        if(htskGamePadPrint)
        {
            tskRemove(htskGamePadPrint);
            htskGamePadPrint=NULL;
        }
        else
        {
            gpPrint(&GPDat, joystick_and_pressure);
        }
    }
    
    return CMD_SUCCESS; 
}

int CMD_GpSpiInit(char * arg)
{
    uint32_t prescaler;
    int r = cmdGetArg(arg, &prescaler);
    if(r!=1) return CMD_ERRARG;

    uint16_t SPI_BaudRatePrescaler;

    switch (prescaler)
    {
    case 2:
        SPI_BaudRatePrescaler=SPI_BAUDRATEPRESCALER_2;  break;
    case 4:
        SPI_BaudRatePrescaler=SPI_BAUDRATEPRESCALER_4;  break;
    case 8:
        SPI_BaudRatePrescaler=SPI_BAUDRATEPRESCALER_8;  break;
    case 16:
        SPI_BaudRatePrescaler=SPI_BAUDRATEPRESCALER_16; break;
    case 32:
        SPI_BaudRatePrescaler=SPI_BAUDRATEPRESCALER_32; break;
    case 64:
        SPI_BaudRatePrescaler=SPI_BAUDRATEPRESCALER_64; break;
    case 128:
        SPI_BaudRatePrescaler=SPI_BAUDRATEPRESCALER_128;break;
    case 256:
        SPI_BaudRatePrescaler=SPI_BAUDRATEPRESCALER_256;break;
    default:
        printf("Illegal prescaler: prescaler must be 2,4,8,16,32,64,128,256\r\n");
        return CMD_FAILURE;
    }

    gpSpiInit(SPI_BaudRatePrescaler);
    return CMD_SUCCESS;
}
int CMD_GpSpiXfer(char * arg)
{
    const uint32_t str_len=128;
    char str[str_len];
    memset(str, 0, str_len);

    int r = cmdGetArg(arg, &str);
    if(r!=1) return CMD_ERRARG;

    /* ��������"0x12,0x34,0x56"��ʽ�Ĳ��������͸�SPI���豸 */

    /* ��һ��һ���ж��ٸ������͵����� */
    size_t datac=1;
    for(int i=0;str[i]!='\0';i++)
    {
        if(str[i]==',' )
        {
            datac++;
        }
    }

    /* ������պͷ����ڴ� */
    uint8_t * data_out = malloc((size_t)datac);
    uint8_t * data_in = malloc((size_t)datac);
    if(!data_out || !data_in)
    {
        printf("malloc fail.\r\n");
        return CMD_ERRMEM;
    }

    /* �Ӳ�������ȡ��Ч�Ĵ��������� */
    char *end, *start = str;
    size_t datac_valid=0;

    for(int pos=0; datac_valid<datac; )
    {
        start=str+pos;

        unsigned long tmp = strtoul(start, &end, 0);
        data_out[datac_valid]=(uint8_t)tmp;

        if(*end==',')
        {
            pos=end-str+1;
            datac_valid++;
            if(str[pos]=='\0') break;
        }
        else if(*end=='\0')
        {
            datac_valid++;
            break;
        }
        else
            break;
    }

    /* ��ӡ��Ч���ݸ�������ֵ */
    printf("Valid data count: %u \r\n", datac_valid);

    printf("data_out: ");
    for(size_t i=0; i<datac_valid; i++)
    {
        printf("0x%02X ", data_out[i]);
    }
    printf("\r\n");

    memset(data_in, 0, datac_valid*sizeof(uint8_t));

    CS_LOW();      //ƬѡNSS����
    gpSpiXfer(data_out, data_in, (uint16_t)datac_valid);
    CS_HIGH();     //ƬѡNSS����

    /* ��ӡ�������� */
    printf("data_in: ");
    for(size_t i=0; i<datac_valid; i++)
    {
        printf("0x%02X ", data_in[i]);
    }
    printf("\r\n");

    free(data_out);
    free(data_in);

    return CMD_SUCCESS;
}

int CMD_GpSpiPullChk(char * arg)
{
    UNUSED(arg);

    gpPullChk();
    return CMD_SUCCESS;
}
int CMD_GpSpiPull(char * arg)
{
    uint32_t vibr0, vibr1;
    int r = cmdGetArg(arg, &vibr0, &vibr1);
    if(r!=2) return CMD_ERRARG;

    gpPull((uint8_t)vibr0, (uint8_t)vibr1, &GPDat);

    return CMD_SUCCESS;
}
int CMD_GpSpiConfig(char * arg)
{
    uint32_t enter;
    int r = cmdGetArg(arg, &enter);
    if(r!=1) return CMD_ERRARG;

    gpConfig(enter);

    return CMD_SUCCESS;    
}
int CMD_GpSpiMode(char * arg)
{
    uint32_t mode, lock;
    int r = cmdGetArg(arg, &mode, &lock);
    if(r!=1) return CMD_ERRARG;

    gpMode(mode, lock);

    return CMD_SUCCESS;
}
int CMD_GpSpiVib(char * arg)
{
    uint32_t motor0, motor1;
    int r = cmdGetArg(arg, &motor0, &motor1);
    if(r!=2) return CMD_ERRARG;

    gpVib(motor0, motor1);

    return CMD_SUCCESS;    
}
int CMD_GpSpiPress(char * arg)
{
    uint32_t arrow, color, trig;
    int r = cmdGetArg(arg, &arrow, &color, &trig);
    if(r!=3) return CMD_ERRARG;

    gpPressure(arrow, color, trig);

    return CMD_SUCCESS;    
}

int CMD_GpTest(char * arg)
{
    char cmd[8][64];
    memset(cmd, 0, sizeof(cmd));

    int r = cmdGetArg(arg, &cmd[0], &cmd[1], &cmd[2], &cmd[3],
                &cmd[4], &cmd[5], &cmd[6], &cmd[7]);
    if(r<=0) return CMD_ERRARG;

    size_t cmd_num=(size_t)r;
    printf("Get %u commands: \r\n", cmd_num);
    for(size_t i=0; i<cmd_num; i++)
    {printf("%s\r\n", cmd[i]);}

    printf("\r\n");

    for(size_t i=0; i<cmd_num; i++)
    {cmdExec(cmd[i]);}

    return CMD_SUCCESS;
}

int regGamepadCmds(void)
{
    int r;
    do
    {
        r = cmdRegister("gpchkinvl", CMD_GpChkInvl, \
                "[invl:ms]: Set the gamepad checking period."); if(r) break;
        r = cmdRegister("gpvib", CMD_GpVib, \
                "[vibr0],[vibr1]: Vibrate the gamepad once."); if(r) break;
        r = cmdRegister("gpprint", CMD_GpPrint, \
                "[invl:print invl in ms]," 
                "[data:0-button|1-joystick|2-joystick and pressure]: " \
                "Init SPI."); if(r) break;

        r = cmdRegister("gpspiinit", CMD_GpSpiInit, \
                "[prescaler:2~256]: init SPI."); if(r) break;

        r = cmdRegister("gpspixfer", CMD_GpSpiXfer, \
                "[data]: Transfer data though SPI."); if(r) break;
        r = cmdRegister("gppullchk", CMD_GpSpiPullChk, \
                "[none]: Check which button in the data."); if(r) break;
        r = cmdRegister("gppull", CMD_GpSpiPull, \
                "[vibr0],[vibr1]: Pull the data."); if(r) break;

        r = cmdRegister("gpconfig", CMD_GpSpiConfig, \
                "[enter:1-enter|0-exit]: enter the config mode."); if(r) break;
        r = cmdRegister("gpmode", CMD_GpSpiMode, \
                "[mode:1-analog|0-digital]: switch between "
                "analog and digital mode."); if(r) break;
        r = cmdRegister("gpvibr", CMD_GpSpiVib, \
                "[small motor:1-vibr|0-novibr]," \
                "[large motor:1-vibr|0-novibr]: " \
                "enable the motor viberation"); if(r) break;
        r = cmdRegister("gppress", CMD_GpSpiPress, \
                "[arror:1-en|0-dis],[color:1-en|0-dis],[trig:1-en|0-dis]: " \
                "enable pressure."); if(r) break;
        r = cmdRegister("gptest", CMD_GpTest, \
                "[cmd0],[cmd1],[cmd2],[cmd3],[cmd4],[cmd5],[cmd6],[cmd7]: " \
                "execute multiple commands."); if(r) break;

    }while(0);

    return r;
}

void DMA1_Stream4_IRQHandler(void);
void DMA1_Stream4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(__gamepad_spi.hdmatx);
}
void DMA1_Stream3_IRQHandler(void);
void DMA1_Stream3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(__gamepad_spi.hdmarx);
}


#pragma GCC diagnostic pop

