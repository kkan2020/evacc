/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Ken W.T. Kan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 

/**
  ******************************************************************************
  * @file    TIM/PWM_Input/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.4.0
  * @date    10/15/2010
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and peripherals
  *          interrupt service routine.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 

/* Includes ------------------------------------------------------------------*/
//#include "stm32f10x_bkp.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "stm32f10x_it.h"
#include "cmsis_os2.h"
#include "hal.h"
#include "i2c_hw.h"
#include "RFID_Card.h"
#include "RFIDThread.h"
#include "CT_Thread.h"
#include "dhThread.h" 

__IO uint8_t RandomBytes[UID_LEN+DEV_UID_LEN], IsRandomBytesReady, CountUpTimerFlag;
__IO uint16_t InjectedGroupIndex;
__IO uint16_t ADC_InjectedConvertedValueTab[10];
__IO uint32_t CountUpTimerCounter, CountDownTimerCounter;

void ResetInjectedGroupIndex(void)
{
  InjectedGroupIndex = 0;
}

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{}

/******************************************************************************/
/*            STM32F10x Peripherals Interrupt Handlers                        */
/******************************************************************************/

extern TI2C I2C2Port;

void I2C2_EV_IRQHandler(void)
{
  EepromHandler(&I2C2Port);
}

void I2C2_ER_IRQHandler(void)
{
  /* Read SR1 register to get I2C error */
  if ((I2C_ReadRegister(I2C2, I2C_Register_SR1) & 0xFF00) != 0x00)
  {
    /* Clears error flags */
    I2C2->SR1 &= 0x00FF;    
  }
}

/**
  * @brief  This function handles TIM4 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM4_IRQHandler(void)
{
  if(TIM_GetITStatus(TIM4, TIM_IT_Update) == SET) 
  {
    EepromTimeoutHandler(&I2C2Port);
    TIM_ClearITPendingBit(TIM4, TIM_IT_Update);    
  }
}

/**
  * @brief  This function handles WIFI_UART_RX_TIM global interrupt request.
  * @param  None
  * @retval None
  */
void TIM6_IRQHandler(void)
{
  TSPort *p;
  if(TIM_GetITStatus(TIM6, TIM_IT_Update) == SET) 
  {
    p = &WiFiPort;
    if (osMessageQueuePut(SPortMsgQ, &p, NULL, 0)!=osOK)
      WiFiPort.RxPos = 0;
    DISABLE_TIMER(TIM6);
    TIM_ClearITPendingBit(TIM6, TIM_IT_Update);    
  }
}

/**
  * @brief  This function handles DMA1_Channel1 global interrupt request.
  * @param  None
  * @retval None
  */
void DMA1_Channel1_IRQHandler(void)
{
  extern osThreadId_t CTThread_id;
  
  if (DMA1->ISR & DMA1_IT_HT1) //half transfer
    osThreadFlagsSet(CTThread_id, CT_HALF_TRANS);
  else if (DMA1->ISR & DMA1_IT_TC1) //transfer complete 
    osThreadFlagsSet(CTThread_id, CT_TRANS_COMP);
  /* Clear Global interrupt pending bits */
  DMA1->IFCR = DMA1_IT_GL1;  
}

/**
  * @brief  This function handles ADC1 and ADC2 global interrupts requests.
  * @param  None
  * @retval None
  */
#if defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || defined (STM32F10X_HD_VL)
void ADC1_IRQHandler(void)
#else
void ADC1_2_IRQHandler(void)
#endif
{
  /* Clear ADC1 JEOC pending interrupt bit */
  ADC_ClearITPendingBit(ADC1, ADC_IT_JEOC);
  
  /* Get injected channel11 and channel12 converted value */
  ADC_InjectedConvertedValueTab[InjectedGroupIndex++] = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);
  
  if (InjectedGroupIndex == CountOf(ADC_InjectedConvertedValueTab))
  {
    ADC_ExternalTrigInjectedConvCmd(ADC1, DISABLE);
    InjectedGroupIndex = 0;
  }
}

/**
  * @brief  This function handles USART2 global interrupt request.
  * @param  None
  * @retval None
  */
/* Handler of RFID card reader */
void USART2_IRQHandler(void)
{
  extern osThreadId_t RFIDThread_id;
  static uint8_t c;
  
  if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
  {    
    c = USART_ReceiveData(USART2) & 0xFF;
    if (UartRFID.RxState==0)
    {
      if (c == 0xAA)
        UartRFID.RxState++;     
    } 
    else if (UartRFID.RxState==1)
    {
      if (c == 0xBB)
      {
        UartRFID.RxState++;
        UartRFID.RxBuffer[0] = 0xAA;
        UartRFID.RxBuffer[1] = 0xBB;
        UartRFID.RxPos = 2;
      }
      else
        UartRFID.RxState = 0;
    }
    else
    {
      if (UartRFID.Rx0)
        UartRFID.Rx0 = 0;
      else 
      {
        if (c==0xAA)
          UartRFID.Rx0 = 1;    
        UartRFID.RxBuffer[UartRFID.RxPos++] = c;
        if (UartRFID.RxPos==3) //size byte
          UartRFID.RxSize = c+4;
      }
      if ((UartRFID.RxSize>0) && (UartRFID.RxPos>=UartRFID.RxSize) && !UartRFID.Rx0)
      {
        USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);
        osThreadFlagsSet(RFIDThread_id, UART_RX_OK);
        UartRFID.RxState = 0;        
      }
    }
  }

  if(USART_GetITStatus(USART2, USART_IT_TXE) != RESET)
  {   
    if (UartRFID.Tx0)
    {
      USART_SendData(USART2, 0);
      UartRFID.Tx0 = 0;
    }
    else
    {
      c = UartRFID.TxBuffer[UartRFID.TxPos++];
      if ((UartRFID.TxPos>1) && (c==0xAA))
        UartRFID.Tx0 = 1;
      USART_SendData(USART2, c);
    }
    if ((UartRFID.TxPos >= UartRFID.TxSize) && !UartRFID.Tx0)
    {
      USART_ITConfig(USART2, USART_IT_TXE, DISABLE);
      USART_ClearITPendingBit(USART2, USART_IT_TC);
      osThreadFlagsSet(RFIDThread_id, UART_TX_OK);
    }
  }
}

/**
  * @brief  This function handles USART1 global interrupt request.
  * @param  None
  * @retval None
  */
/* Handler of SOCKET 2 port (GPRS) */
void USART1_IRQHandler(void)
{
}

/**
  * @brief  This function handles UART4 global interrupt request.
  * @param  None
  * @retval None
  */
/* Handler of SOCKET 1 port (WIFI) */
void UART4_IRQHandler(void)
{
  if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)
  {
    if (!IS_TIMER_ENABLED(WIFI_UART_RX_TIM))
      WiFiPort.RxPos = 0;
    if (WiFiPort.RxPos>=sizeof(WiFiPort.RxBuffer))		
      WiFiPort.RxPos = 0;
		WiFiPort.RxBuffer[WiFiPort.RxPos++] = USART_ReceiveData(UART4) & 0xFF;  
    SetTimeout_us(WIFI_UART_RX_TIM, 5000);
  }

  if (USART_GetITStatus(UART4, USART_IT_TXE) != RESET)
  {   
    if (WiFiPort.TxPos < WiFiPort.TxSize)
      USART_SendData(UART4, WiFiPort.TxBuffer[WiFiPort.TxPos++]);
		if (WiFiPort.TxPos >= WiFiPort.TxSize)
		{
      WiFiPort.TxSize = 0;
      USART_ITConfig(UART4, USART_IT_TXE, DISABLE);
      USART_ClearITPendingBit(UART4, USART_IT_TC);
		}
  }
}


/*******************************************************************************
* Function Name  : RTC_IRQHandler
* Description    : This function handles RTC interrupt request which is triggered every second.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void RTC_IRQHandler(void)
{
  static int ii=0;
  
  //read system tick counter values using as seed of private key
  if (ii > sizeof(RandomBytes))
    ii = 0;
  if (ii!=0)
  {
    RandomBytes[ii-1] = SysTick->VAL;
    if (ii==sizeof(RandomBytes))
      IsRandomBytesReady = 1;    
/*    
    uint16_t t;
    srand(SysTick->VAL);
    do
    {
      t = rand() & 0xff;
    } while (!(isalpha(t) || isdigit(t) || (t=='-')));
    RandomBytes[ii-1] = t;
    if (ii==sizeof(RandomBytes))
      IsRandomBytesReady = 1;    
*/    
  }
  ii++;
  
  if (!CountUpTimerFlag)
    CountUpTimerCounter++;
  
  if (CountDownTimerCounter)
    CountDownTimerCounter--;
  
  NVIC_ClearPendingIRQ(RTC_IRQn);
  RTC_ClearITPendingBit(RTC_IT_SEC);  
}


void CountUpTimerReset(void)
{
  MASK_IRQ
  CountUpTimerCounter = 0;
  UNMASK_IRQ
}

void CountUpTimerStop(void)
{
  MASK_IRQ
  CountUpTimerFlag = 1;
  UNMASK_IRQ
}
  
void CountUpTimerStart(void)
{
  MASK_IRQ
  CountUpTimerFlag = 0;
  UNMASK_IRQ
}

uint32_t CountUpTimerGet(void)
{
  MASK_IRQ
  uint32_t ret = CountUpTimerCounter;
  UNMASK_IRQ
  return ret;
}

void DlyTimerSet(uint32_t seconds)
{
  MASK_IRQ
  CountDownTimerCounter = seconds;
  UNMASK_IRQ
}

int IsDlyTimerStopped(void)
{
  MASK_IRQ
  int ret = CountDownTimerCounter==0? 1: 0;
  UNMASK_IRQ
  return ret;
}


