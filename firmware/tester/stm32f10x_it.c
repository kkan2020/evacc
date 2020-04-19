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
#include "stm32f10x_it.h"
#include "hal.h"

__IO uint16_t IC2Value = 0;
__IO float PWM_DutyCycle = 0;
__IO uint32_t PWM_Freq = 0;
__IO uint16_t CCResistance = 0; 


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

/**
  * @brief  This function handles TIM2 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM2_IRQHandler(void)
{
  /* Clear Capture compare interrupt pending bit */  
  TIO_TIMER->CNT = 0; //reset timeout timer
  TIM2->SR = (uint16_t)~TIM_IT_CC2;
  
  /* Disable timeout timer */
  TIO_TIMER->CCR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
  
  /* Get the Input Capture value */
  IC2Value = TIM_GetCapture2(TIM2);

  if (IC2Value != 0)
  {
    /* Duty cycle computation */
    PWM_DutyCycle = ((float)TIM_GetCapture1(TIM2) * 100) / IC2Value;

    /* Frequency computation */
    PWM_Freq = ICP_TIMER_FREQ / IC2Value;
  }
  else
  {
    PWM_DutyCycle = 0;
    PWM_Freq = 0;
  }
  //disable timer
//  TIM2->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
}

/**
  * @brief  This function handles TIM4 global interrupt request.
  * @param  None
  * @retval None
  * @desc   Called when PWM input capture timeout
  */
void TIM4_IRQHandler(void)
{
  if(TIM_GetITStatus(TIM4, TIM_IT_Update) == SET) 
  {
    /* Disable pilot signal measurement timer*/
//    ICP_TIMER->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
    PWM_DutyCycle = 0;
    PWM_Freq = 0;
    
    /* Reset DMA channel */
//    DMA1_Channel1->CCR &= (uint16_t)(~DMA_CCR1_EN);
    
    /* Clear interrupt pending bit*/
    TIM4->SR = (uint16_t)~TIM_IT_Update;
    //disable timer
//    TIM4->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
  }
}

/**
  * @brief  This function handles DMA1_Channel1 global interrupt request.
  * @param  None
  * @retval None
  */
void DMA1_Channel1_IRQHandler(void)
{
  uint16_t i, s, valid = 0;
  float u;
  
  if (DMA1->ISR & DMA1_IT_HT1) //half transfer
  {
    valid = 1;
    for (i=s=0; i<CountOf(ADCConvertedValues)/2; i++)
      s += ADCConvertedValues[i];
  }
  else if (DMA1->ISR & DMA1_IT_TC1) //transfer complete 
  {
    valid = 1;
    for (s=0, i=CountOf(ADCConvertedValues)/2; i<CountOf(ADCConvertedValues); i++)
      s += ADCConvertedValues[i];
  }
  if (valid)
  {
    s /= (CountOf(ADCConvertedValues)>>1); //average
    u = s*3.3/4095;
    if (u < VREF)
      CCResistance = u*RCC_PULLUP/(VREF-u) + 0.5;
    else
      CCResistance = 0xffff; //infinity    
  }
  /* Clear Global interrupt pending bits */
  DMA1->IFCR = DMA1_IT_GL1;  
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 

/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
