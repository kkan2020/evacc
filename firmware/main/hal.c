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
 
#include <string.h>
#include <stdlib.h>
#include "stm32f10x.h"
#include "hal.h"
#include "stm32f10x_it.h"

TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

void RCC_Configuration(void)
{
  /* APB2 Periph clock enable */							
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|
    RCC_APB2Periph_GPIOD|RCC_APB2Periph_AFIO|RCC_APB2Periph_TIM1, ENABLE);

  RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
  
  /* APB1 Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2|RCC_APB1Periph_TIM3|RCC_APB1Periph_TIM4|\
    RCC_APB1Periph_TIM6|RCC_APB1Periph_TIM7|RCC_APB1Periph_I2C2, ENABLE);  
  
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);
  
    /* Enable DMA1 clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  
  /* ADCCLK = PCLK2/8 */
  RCC_ADCCLKConfig(RCC_PCLK2_Div8); 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
}

void GPIO_Configuration(void)
{
  static GPIO_InitTypeDef GPIO_InitStructure;

  /* Configure all unused GPIO port pins in Analog Input mode (floating input
     trigger OFF), this will reduce the power consumption and increase the device
     immunity against EMI/EMC *************************************************/
  /* preserve JTAG pins */
#ifdef _DEBUG  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All & ~(GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15);
#else
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
#endif
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

#ifdef _DEBUG  
#ifdef USE_SWJ 
  GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE); //disable JTAG, enable SWJ
#endif
#endif
  
  /* preserve JTAG pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All & ~(GPIO_Pin_3 | GPIO_Pin_4);
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_All;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_Init(GPIOD, &GPIO_InitStructure);

/********* PORT A ***************/
  /* config UART1,2 Rx pins, UART1.CTS pin, Pilot pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_7|GPIO_Pin_10|GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* config UART1,2 Tx pins, PWM, UART1.RTS pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);	

/* config Input PU pins: debug pin */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* config Output PP pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  
  /* config pilot ADC pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

/********* PORT B ***************/
  /* config Input PU pins */

  /* config Output PP pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|
                                GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_12;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* config I2C2 pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11;  //SCL, SDA pin
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
/********* PORT C ***************/  
  /* config pilot ADC pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* config Output PP pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_8|
                                GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  /* config Input PU pins: em_stop, ir_tx */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9|GPIO_Pin_14;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);  
  
  /* config open drain pins: VTemp */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);  

  /* config UART4 Rx pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  /* config UART4 Tx pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);	

  /********* PORT D ***************/
  /* config output PP pins: BLA */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void TIM_Configuration(void)
{
  uint32_t tim_clk_freq;

  /* ADC trigger timer configuration ------------------------------------------------------
     TRGO is used to trigger ADC to start conversion
  ------------------------------------------------------------- */
  tim_clk_freq = 1000000;
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure); 
  TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/tim_clk_freq-1;
  TIM_TimeBaseStructure.TIM_Period = tim_clk_freq/CT_SAMPLE_FREQ-1;         
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;    
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(ADC1_TIMER, &TIM_TimeBaseStructure);
  //select update event as the trigger output (TRGO)
  TIM_SelectOutputTrigger(ADC1_TIMER, TIM_TRGOSource_Update);   
//  TIM_UpdateRequestConfig(ADC1_TIMER, TIM_UpdateSource_Global); 
//  TIM_UpdateDisableConfig(ADC1_TIMER, DISABLE);
  
  /* I2C2 timeout timer configuration ------------------------------------------------------
     timeout timer for I2C
  ------------------------------------------------------------- */
  tim_clk_freq = 2000;
  TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/tim_clk_freq-1;
  TIM_TimeBaseStructure.TIM_Period = tim_clk_freq/CT_SAMPLE_FREQ-1;         
  TIM_TimeBaseInit(I2C2_TIMER, &TIM_TimeBaseStructure);
  TIM_ClearITPendingBit(I2C2_TIMER, TIM_IT_Update);
  TIM_ITConfig(I2C2_TIMER, TIM_IT_Update, ENABLE);  
  
  /* DS18B20 timer configuration ------------------------------------------------------
     timeout timer for 1-wire interface
  ------------------------------------------------------------- */
  tim_clk_freq = 1000000;
  TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/tim_clk_freq-1;
  TIM_TimeBaseStructure.TIM_Period = 1;         
  TIM_TimeBaseInit(US_TIMER, &TIM_TimeBaseStructure);
  TIM_ClearITPendingBit(US_TIMER, TIM_IT_Update);
  
/* UART 6,7 timer configuration ------------------------------------------------------
     timeout timers for WIFI_UART_RX and GPRS UART interface
  ------------------------------------------------------------- */
  TIM_TimeBaseInit(WIFI_UART_RX_TIM, &TIM_TimeBaseStructure);
  TIM_ClearITPendingBit(WIFI_UART_RX_TIM, TIM_IT_Update);
  TIM_ITConfig(WIFI_UART_RX_TIM, TIM_IT_Update, ENABLE);  
  
  TIM_TimeBaseInit(GPRS_UART_TIMER, &TIM_TimeBaseStructure);
  TIM_ClearITPendingBit(GPRS_UART_TIMER, TIM_IT_Update);
  TIM_ITConfig(GPRS_UART_TIMER, TIM_IT_Update, ENABLE);  
}

void PWM_Configuration(void)
{
  static TIM_OCInitTypeDef  TIM_OCInitStructure;
  uint16_t PrescalerValue = 0;

  /* -----------------------------------------------------------------------

  The TIM1CLK frequency is set to SystemCoreClock (72 MHz), to get TIMx counter
  clock at 1 MHz the Prescaler is computed as following:
   - Prescaler = (TIMxCLK / TIMx counter clock) - 1

  The TIMx is running at 1MHz: TIMx Frequency = TIMx counter clock/(ARR + 1)
                                                = 1 MHz / (999+1) = 1 KHz
  The TIMx CCR1 register value is equal to (ARR+1)/2, so the TIMx Channel 1 generates a 
  PWM signal with a frequency equal to 1 KHz and a duty cycle equal to 50%:
  TIM1 Channel1 duty cycle = (TIM1_CCR1/ (TIM1_ARR + 1))* 100 = 50%
*/

  PrescalerValue = (uint16_t) (SystemCoreClock / 1000000) - 1;
  /* Time base configuration */
  TIM_TimeBaseStructure.TIM_Period = PWM_FREQ-1;
  TIM_TimeBaseStructure.TIM_Prescaler = PrescalerValue;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(PWM_TIMER, &TIM_TimeBaseStructure);
  TIM_UpdateRequestConfig(PWM_TIMER, TIM_UpdateSource_Global); 

  /* PWM1 Mode configuration: Channel1 */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;
  TIM_OCInitStructure.TIM_Pulse = PWM_FREQ; //output always high
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC1Init(PWM_TIMER, &TIM_OCInitStructure);
  
  TIM_OC1PreloadConfig(PWM_TIMER, TIM_OCPreload_Enable);
  TIM_ARRPreloadConfig(PWM_TIMER, ENABLE);
  /* TIMER Main Output Enable */
  TIM_CtrlPWMOutputs(PWM_TIMER, ENABLE);      
  
  /* generate update event to update registers */
  TIM_GenerateEvent(PWM_TIMER, TIM_EventSource_Update);
  
  /* start PWM timer */
  TIM_Cmd(PWM_TIMER, ENABLE);  
}

#ifdef STM32F10X_MD_VL
  #define ADC_IRQ               ADC1_IRQn         
#else
  #define ADC_IRQ               ADC1_2_IRQn
#endif

void NVIC_Configuration(void)
{
  static NVIC_InitTypeDef NVIC_InitStructure;

  memset(&NVIC_InitStructure, 0, sizeof(NVIC_InitStructure));
  
#if defined (STM32F10X_LD_VL) || defined (STM32F10X_MD_VL) || defined (STM32F10X_HD_VL)
  NVIC_InitStructure.NVIC_IRQChannel = ADC1_IRQn;
#else
  NVIC_InitStructure.NVIC_IRQChannel = ADC1_2_IRQn;
#endif
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
    
  /* Configure and enable I2C2 interrupt -------------------------------------*/
  NVIC_InitStructure.NVIC_IRQChannel = I2C2_EV_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  /* Enable the DMA1_Channel1 gobal Interrupt ------------------*/
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);   

  /* Configure and enable I2C2 error interrupt -------------------------------------*/
  NVIC_InitStructure.NVIC_IRQChannel = I2C2_ER_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Configure and enable I2C timeout interrupt ------------------*/ 
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 

  /* Enable the UART4 Interrupt --------- SOCKET1 */
  NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
 
 /* Enable the USART1 Interrupt -------- SOCKET2 */
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
 
  /* Configure and enable TIM6 update interrupt ------------------*/ 
  NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 

  /* Configure and enable TIM7 update interrupt ------------------*/ 
  NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 

  /* Enable the USART2 Interrupt -------- RFID */
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

/* Configure and enable RTC interrupt -----------------------------------------*/ 
  NVIC_InitStructure.NVIC_IRQChannel = RTC_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);   
}

void DMA_Configuration(void)
{
  static DMA_InitTypeDef DMA_InitStructure;
  /* DMA1 channel1 configuration ----------------------------------------------*/
  DMA_DeInit(DMA1_Channel1);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)CT_ADCVales;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = CountOf(CT_ADCVales);
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
  DMA_InitStructure.DMA_Priority = DMA_Priority_High;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel1, &DMA_InitStructure);
  
  /* Enable DMA1 channel1 */
  DMA_Cmd(DMA1_Channel1, ENABLE);
  DMA_ITConfig(DMA1_Channel1, DMA_IT_HT|DMA_IT_TC, ENABLE);
}

__IO int16_t CT_ADCVales[CT_ADC_BUFFER_SIZE];

void ADC_Configuration(void)
{
  static ADC_InitTypeDef  ADC_InitStructure;

  /* ADC1 Configuration ------------------------------------------------------*/
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = ENABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; 
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 3;
  ADC_Init(ADC1, &ADC_InitStructure);

  /* ADC1 regular channels configuration */ 
  ADC_RegularChannelConfig(ADC1, ADC_CHNL_CT1, 1, ADC_SampleTime_28Cycles5); 
  ADC_RegularChannelConfig(ADC1, ADC_CHNL_CT2, 2, ADC_SampleTime_28Cycles5); 
  ADC_RegularChannelConfig(ADC1, ADC_CHNL_CT3, 3, ADC_SampleTime_28Cycles5); 

  /* Enable ADC1 external trigger conversion */ 
  ADC_ExternalTrigConvCmd(ADC1, ENABLE);
  
  /* Set injected sequencer length */
  ADC_InjectedSequencerLengthConfig(ADC1, 1);
  /* injected channel configuration */ 
  ADC_InjectedChannelConfig(ADC1, ADC_CHNL_PILOT, 1, ADC_SampleTime_239Cycles5);
  /* injected external trigger configuration */
  ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO);

  /* Disable injected external trigger conversion */
  ADC_ExternalTrigInjectedConvCmd(ADC1, DISABLE);
  
  /* Enable JEOC interupt */
  ADC_ITConfig(ADC1, ADC_IT_JEOC, ENABLE);
  
  /* Enable ADC DMA */
  ADC_DMACmd(ADC1, ENABLE);
  
  /* Enable ADC */
  ADC_Cmd(ADC1, ENABLE);

  /* Enable ADC reset calibaration register */   
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC calibaration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC calibration */
  while(ADC_GetCalibrationStatus(ADC1));    
}


void I2C_Configuration(void)
{
  static I2C_InitTypeDef  I2C_InitStructure;

  I2C_DeInit(I2C2);
  I2C_Cmd(I2C2, DISABLE);                               
  /* I2C2 configuration ------------------------------------------------------*/
  I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
  I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
  I2C_InitStructure.I2C_OwnAddress1 = I2C_OWN_ADDR7;
  I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
  I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
  I2C_InitStructure.I2C_ClockSpeed = I2C_CLK_SPEED;
  I2C_Init(I2C2, &I2C_InitStructure);
  I2C_Cmd(I2C2, ENABLE);
}

void USART_Configuration(void)
{
  static USART_InitTypeDef USART_InitStructure;

  /* RFID PORT */
  USART_InitStructure.USART_BaudRate = 9600;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
  USART_Init(USART2, &USART_InitStructure);
  USART_Cmd(USART2, ENABLE);
  
  /* SOCKET 2 PORT (GPRS) */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
  USART_Init(USART1, &USART_InitStructure);
//  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
//  USART_Cmd(USART1, ENABLE);  
  
  /* SOCKET 1 PORT (WIFI) */
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
  USART_Init(UART4, &USART_InitStructure);
  USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
//  USART_Cmd(UART4, ENABLE);
}


void RTC_Configuration(void)
{
  /* Enable PWR and BKP clocks */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);

  /* Allow access to BKP Domain */
  PWR_BackupAccessCmd(ENABLE);

  /* Reset Backup Domain */
  BKP_DeInit();

  /* Enable the LSI OSC */
  RCC_LSICmd(ENABLE);
  /* Wait till LSI is ready */
  while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {}
  /* Select the RTC Clock Source */
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);

  /* Enable RTC Clock */
  RCC_RTCCLKCmd(ENABLE);

  /* Wait for RTC registers synchronization */
  RTC_WaitForSynchro();

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();

  /* Enable the RTC Second */
  RTC_ITConfig(RTC_IT_SEC, ENABLE);

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();

  /* Set RTC prescaler: set RTC period to 1sec */
  RTC_SetPrescaler(39999);

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForLastTask();
}

void IWDG_Configuration(void)
{
 /* IWDG timeout equal to 280 ms (the timeout may varies due to LSI frequency
     dispersion) */
  /* Enable write access to IWDG_PR and IWDG_RLR registers */
  IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

  /* IWDG counter clock: 40KHz(LSI) / 32 = 1.25 KHz */
  IWDG_SetPrescaler(IWDG_Prescaler_16);

  /* Set counter reload value */
  IWDG_SetReload(0xFFF);  //time out at 1638ms

  /* Reload IWDG counter */
  IWDG_ReloadCounter();

  /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
  IWDG_Enable();
}


void ResetWDT(void)
{
#ifdef WDT_ENABLE
  IWDG_ReloadCounter();
#endif
}

static __IO float PWM_DutyCycle;

float PWM_GetDutyCycle(void)
{
  return PWM_DutyCycle;
}

int PWM_IsDC(void)
{
  return (PWM_DutyCycle==0.0)||(PWM_DutyCycle==1.0);
}

void PWM_SetDutyCycle(float DutyCycle)
{
  if ((DutyCycle<0.0)||(DutyCycle>1.0))
    return;
  PWM_DutyCycle = DutyCycle;
  PWM_TIMER->CCR1 = PWM_DutyCycle*(PWM_FREQ);
}

void PWM_ExtTriggerDisable(void)
{
  ADC_ExternalTrigInjectedConvCmd(ADC1, DISABLE);
}

void PWM_ExtTriggerEnable(void)
{
  ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);
}

/*
*  Only apply to timers with counter clock frequency 1000000Hz
*/
void SetTimeout_us(TIM_TypeDef* TIMx, uint16_t us)
{
  DISABLE_TIMER(TIMx);
  TIMx->CNT = 0; //reset counter
  TIMx->ARR = us-1; //set auto-reload register
  TIMx->SR = (uint16_t)~TIM_FLAG_Update; //clear update flag
  ENABLE_TIMER(TIMx);
}

void WiFi_Reset(void)
{
  uint32_t t;
  
  SO1_RESET(0);
  for (t=0; t<100000; t++)
    __nop();
  SO1_RESET(1);
}

void GPRS_Configuration(void)
{
  uint32_t t;
  SO2_EN(1);
  SO2_SLEEP(1);
  for (t=0; t<5000000; t++) __nop();
  SO2_RESET(1); //inverted by FET
  for (t=0; t<100000; t++) __nop();
  SO2_RESET(0); //inverted by FET
}

void LedSetState(int mask, int state)
{
  if (mask & LED_READY_BIT)
    LED_SET_READY(state);
  if (mask & LED_CHARGE_BIT)
    LED_SET_CHARGE(state);
  if (mask & LED_FAULT_BIT)
    LED_SET_FAULT(state);
}

void LedToggleState(int mask)
{
  if (mask & LED_READY_BIT)
    LED_TOGGLE_READY;
  if (mask & LED_CHARGE_BIT)
    LED_TOGGLE_CHARGE;
  if (mask & LED_FAULT_BIT)
    LED_TOGGLE_FAULT;
}

void SetLcdCtrlPins(int x)
{
	if (x & LCD_RS) LcdRS_Pin(1);
//	if (x & LCD_RD) LcdRD_Pin(1);
  if (x & LCD_WR) LcdWR_Pin(1);
	if (x & LCD_CS1) LcdCS_Pin(1);
	if (x & LCD_RES) 
		LcdRES_Pin(1);
}

void ClrLcdCtrlPins(int x) 
{
	if (x & LCD_RS) LcdRS_Pin(0);
//	if (x & LCD_RD) LcdRD_Pin(0);
  if (x & LCD_WR) LcdWR_Pin(0);
	if (x & LCD_CS1) LcdCS_Pin(0);
	if (x & LCD_RES) 
		LcdRES_Pin(0);
}


