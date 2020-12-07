/**
  ******************************************************************************
  * @file    stm32f4xx_hal_msp_template.c
  * @author  MCD Application Team
  * @version V1.5.0
  * @date    06-May-2016
  * @brief   This file contains the HAL System and Peripheral (PPP) MSP initialization
  *          and de-initialization functions.
  *          It should be copied to the application folder and renamed into 'stm32f4xx_hal_msp.c'.           
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

#include <assert.h>
#include <stdlib.h>

// [ILG]
#if defined ( __GNUC__ )
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#endif

/** @addtogroup STM32F4xx_HAL_Driver
  * @{
  */

/** @defgroup HAL_MSP HAL MSP
  * @brief HAL MSP module.
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/** @defgroup HAL_MSP_Private_Functions HAL MSP Private Functions
  * @{
  */

/**
  * @brief  Initializes the Global MSP.
  * @note   This function is called from HAL_Init() function to perform system
  *         level initialization (GPIOs, clock, DMA, interrupt).
  * @retval None
  */
void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

/**
  * @brief  DeInitializes the Global MSP.
  * @note   This functiona is called from HAL_DeInit() function to perform system
  *         level de-initialization (GPIOs, clock, DMA, interrupt).
  * @retval None
  */
void HAL_MspDeInit(void)
{

}

/**
  * @brief  Initializes the PPP MSP.
  * @note   This functiona is called from HAL_PPP_Init() function to perform 
  *         peripheral(PPP) system level initialization (GPIOs, clock, DMA, interrupt)
  * @retval None
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  if(huart->Instance == USART1)
  {
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA9:TX,  PA10:RX */
    GPIO_InitTypeDef GPIO_InitStruct={0};
    GPIO_InitStruct.Pin = GPIO_PIN_9|GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* work with dma, enable idle intrrupt only. */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);

    /*Initlialize the DMA res. */
    __HAL_RCC_DMA2_CLK_ENABLE();

    DMA_HandleTypeDef * hUartTxDma = malloc(sizeof(DMA_HandleTypeDef));
    DMA_HandleTypeDef * hUartRxDma = malloc(sizeof(DMA_HandleTypeDef));
    assert(hUartTxDma);
    assert(hUartRxDma);

    /* DMA configuration for uart tx */
    hUartTxDma->Instance     = DMA2_Stream7;
    hUartTxDma->Init.Channel = DMA_CHANNEL_4;

    hUartTxDma->Init.Mode      = DMA_NORMAL;
    hUartTxDma->Init.Priority  = DMA_PRIORITY_LOW;
    hUartTxDma->Init.FIFOMode  = DMA_FIFOMODE_DISABLE;

    hUartTxDma->Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hUartTxDma->Init.PeriphInc           = DMA_PINC_DISABLE;
    hUartTxDma->Init.MemInc              = DMA_MINC_ENABLE;
    hUartTxDma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hUartTxDma->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    HAL_DMA_Init(hUartTxDma);

    __HAL_LINKDMA(huart, hdmatx, *hUartTxDma);

    /* Need tx complete intrrupt to free the DMA res. */
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

    /* DMA configuration for uart rx */
    hUartRxDma->Instance     = DMA2_Stream5;
    hUartRxDma->Init.Channel = DMA_CHANNEL_4;
    
    hUartRxDma->Init.Mode      = DMA_CIRCULAR;
    hUartRxDma->Init.Priority  = DMA_PRIORITY_HIGH;
    hUartRxDma->Init.FIFOMode  = DMA_FIFOMODE_DISABLE;

    hUartRxDma->Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hUartRxDma->Init.PeriphInc           = DMA_PINC_DISABLE;
    hUartRxDma->Init.MemInc              = DMA_MINC_ENABLE;
    hUartRxDma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hUartRxDma->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    HAL_DMA_Init(hUartRxDma);

    __HAL_LINKDMA(huart, hdmarx, *hUartRxDma);

    /* Need rx complelte intrrupt to move data. */
    HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
  }
}

/**
  * @brief  DeInitializes the PPP MSP.
  * @note   This functiona is called from HAL_PPP_DeInit() function to perform 
  *         peripheral(PPP) system level de-initialization (GPIOs, clock, DMA, interrupt)
  * @retval None
  */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
  UNUSED(huart);
}

/**
  * @brief  Initializes the PPP MSP.
  * @note   This functiona is called from HAL_PPP_Init() function to perform 
  *         peripheral(PPP) system level initialization (GPIOs, clock, DMA, interrupt)
  * @retval None
  */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
  if(hspi->Instance == SPI2)
  {
    __HAL_RCC_SPI2_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB12:NSS, PB13:CLK, PB14:MISO, PB15:MOSI */
    GPIO_InitTypeDef GPIO_InitStruct={0};
    GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PB12:CS as GPO */ 
    GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Initlialize the DMA res. */
    __HAL_RCC_DMA1_CLK_ENABLE();

    DMA_HandleTypeDef * hSpiTxDma = malloc(sizeof(DMA_HandleTypeDef));
    DMA_HandleTypeDef * hSpiRxDma = malloc(sizeof(DMA_HandleTypeDef));
    assert(hSpiTxDma);
    assert(hSpiRxDma);

    /* DMA configuration for spi tx */
    hSpiTxDma->Instance     = DMA1_Stream4;
    hSpiTxDma->Init.Channel = DMA_CHANNEL_0;

    hSpiTxDma->Init.Mode      = DMA_NORMAL;
    hSpiTxDma->Init.Priority  = DMA_PRIORITY_LOW;
    hSpiTxDma->Init.FIFOMode  = DMA_FIFOMODE_DISABLE;

    hSpiTxDma->Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hSpiTxDma->Init.PeriphInc           = DMA_PINC_DISABLE;
    hSpiTxDma->Init.MemInc              = DMA_MINC_ENABLE;
    hSpiTxDma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hSpiTxDma->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    HAL_DMA_Init(hSpiTxDma);

    __HAL_LINKDMA(hspi, hdmatx, *hSpiTxDma);

    /* Need tx complete intrrupt to free the DMA res. */
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

    /* DMA configuration for Spi rx */
    hSpiRxDma->Instance     = DMA1_Stream3;
    hSpiRxDma->Init.Channel = DMA_CHANNEL_0;
    
    hSpiRxDma->Init.Mode      = DMA_NORMAL;
    hSpiRxDma->Init.Priority  = DMA_PRIORITY_HIGH;
    hSpiRxDma->Init.FIFOMode  = DMA_FIFOMODE_DISABLE;

    hSpiRxDma->Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hSpiRxDma->Init.PeriphInc           = DMA_PINC_DISABLE;
    hSpiRxDma->Init.MemInc              = DMA_MINC_ENABLE;
    hSpiRxDma->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hSpiRxDma->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    HAL_DMA_Init(hSpiRxDma);

    __HAL_LINKDMA(hspi, hdmarx, *hSpiRxDma);

    /* Need rx complelte intrrupt to move data. */
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  }
}

/**
  * @brief  DeInitializes the PPP MSP.
  * @note   This functiona is called from HAL_PPP_DeInit() function to perform 
  *         peripheral(PPP) system level de-initialization (GPIOs, clock, DMA, interrupt)
  * @retval None
  */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
  UNUSED(hspi);
}

/**
  * @brief  Initializes the TIM Output Compare MSP.
  * @param  htim: pointer to a TIM_HandleTypeDef structure that contains
  *                the configuration information for TIM module.
  * @retval None
  */
void HAL_TIM_OC_MspInit(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM4)
  {
    __HAL_RCC_TIM4_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    /* PB6: TIM4_CH1
       PB7: TIM4_CH2
       PB8: TIM4_CH3
       PB9: TIM4_CH4 */
    GPIO_InitTypeDef GPIO_InitStruct={0};
    GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM4;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  }
}

/**
  * @brief  DeInitializes TIM Output Compare MSP.
  * @param  htim: pointer to a TIM_HandleTypeDef structure that contains
  *                the configuration information for TIM module.
  * @retval None
  */
void HAL_TIM_OC_MspDeInit(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);
  /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_TIM_OC_MspDeInit could be implemented in the user file
   */
}

// [ILG]
#if defined ( __GNUC__ )
#pragma GCC diagnostic pop
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
