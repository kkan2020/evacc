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

void RCC_Configuration(void)
{
  /* APB2 Periph clock enable */							
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC|
    RCC_APB2Periph_GPIOD|RCC_APB2Periph_AFIO, ENABLE);

  /* APB1 Periph clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2|RCC_APB1Periph_TIM3|RCC_APB1Periph_TIM4, ENABLE);  
  
    /* Enable DMA1 clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
  
  /* ADCCLK = PCLK2/8 */
  RCC_ADCCLKConfig(RCC_PCLK2_Div8); 
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
}

 void TIM_Configuration(void)
{
  uint32_t tim_clk_freq;
  static TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
  static TIM_ICInitTypeDef TIM_ICInitStructure;

  /* TIM3 configuration ------------------------------------------------------
     TRGO is used to trigger ADC to start conversion
  ------------------------------------------------------------- */
  tim_clk_freq = 2000;
  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure); 
  TIM_TimeBaseStructure.TIM_Prescaler = SystemCoreClock/tim_clk_freq-1;
  TIM_TimeBaseStructure.TIM_Period = tim_clk_freq/ADC_SAMPLE_FREQ-1;         
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;    
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
  TIM_TimeBaseInit(ADC_TIMER, &TIM_TimeBaseStructure);
  //select update event as the trigger output (TRGO)
  TIM_SelectOutputTrigger(ADC_TIMER, TIM_TRGOSource_Update);   
  
  /* TIM4 configuration ------------------------------------------------------
     timeout timer for CP signal measurment
  ------------------------------------------------------------- */
  TIM_TimeBaseStructure.TIM_Period = 0;
  TIM_TimeBaseInit(TIO_TIMER, &TIM_TimeBaseStructure);
  TIM_ClearITPendingBit(TIO_TIMER, TIM_IT_Update);
  TIM_ITConfig(TIO_TIMER, TIM_IT_Update, ENABLE);  
  TIM_SelectOutputTrigger(TIO_TIMER, TIM_TRGOSource_Update);
  TIM_UpdateRequestConfig(TIO_TIMER, TIM_UpdateSource_Global); 
  
  /* TIM2 configuration: Measure external PWM signal ---------------------
     The external signal is connected to TIM2_CH2 pin (PA1)  
     The CCR1 is used to capture period
     The CCR2 is used to capture pulse width 
  ------------------------------------------------------------ */
  TIM_PrescalerConfig(ICP_TIMER, SystemCoreClock/ICP_TIMER_FREQ-1, TIM_PSCReloadMode_Immediate);  
  TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
  TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM_ICInitStructure.TIM_ICFilter = 0x00;

  TIM_PWMIConfig(ICP_TIMER, &TIM_ICInitStructure);

  /* Select the TIMER Input Trigger: TI2FP2 */
  TIM_SelectInputTrigger(ICP_TIMER, TIM_TS_TI2FP2);

  /* Select the slave Mode: Reset Mode */
  TIM_SelectSlaveMode(ICP_TIMER, TIM_SlaveMode_Reset);

  /* Enable the Master/Slave Mode */
  TIM_SelectMasterSlaveMode(ICP_TIMER, TIM_MasterSlaveMode_Enable);
  
  TIM_Cmd(ICP_TIMER, ENABLE);
  
/* Enable the CC2 Interrupt Request */
  TIM_ITConfig(ICP_TIMER, TIM_IT_CC2, ENABLE); 
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
  /* config input capture pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

/* config ADC pins */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

/********* PORT B ***************/
  /* config relay control pins*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12|GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  /* config key column select */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

  /* config key row inputs */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);

/********* PORT C ***************/
  /* config output pins */
  GPIO_InitStructure.GPIO_Pin = (uint16_t)~(GPIO_Pin_14|GPIO_Pin_15);
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  GPIO_SetBits(GPIOC, GPIO_InitStructure.GPIO_Pin);
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
  
  /* Enable the TIM2 input capture Interrupt measure pulse width ------------------*/
  NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 
  
/* Configure and enable TIO_TMR interrupt -- CP signal measurment timeout event-------*/ 
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure); 
  
  /* Enable the DMA1_Channel1 gobal Interrupt ------------------*/
  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);   
}

__IO uint16_t ADCConvertedValues[20];

void DMA_Configuration(void)
{
  static DMA_InitTypeDef DMA_InitStructure;
  /* DMA1 channel1 configuration ----------------------------------------------*/
  DMA_DeInit(DMA1_Channel5);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
  DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADCConvertedValues;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = CountOf(ADCConvertedValues);
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

void ADC_Configuration(void)
{
  static ADC_InitTypeDef  ADC_InitStructure;

  /* ADC1 Configuration ------------------------------------------------------*/
  ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; 
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfChannel = 1;
  ADC_Init(ADC1, &ADC_InitStructure);

  /* ADC1 regular channels configuration */ 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_6, 1, ADC_SampleTime_71Cycles5); //Car_CC

  ADC_ExternalTrigConvCmd(ADC1, ENABLE);
  
  /* Enable ADC1 DMA */
  ADC_DMACmd(ADC1, ENABLE);
  
  /* Enable ADC1 */
  ADC_Cmd(ADC1, ENABLE);

  /* Enable ADC1 reset calibaration register */   
  ADC_ResetCalibration(ADC1);
  /* Check the end of ADC1 reset calibration register */
  while(ADC_GetResetCalibrationStatus(ADC1));

  /* Start ADC1 calibaration */
  ADC_StartCalibration(ADC1);
  /* Check the end of ADC1 calibration */
  while(ADC_GetCalibrationStatus(ADC1));
  
  /* Start ADC1 Software Conversion */ 
//  ADC_SoftwareStartConvCmd(ADC1, ENABLE);
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



void MeasurePilotSignalStart(void){
//  DMA1_Channel1->CCR |= DMA_CCR1_EN;
  ICP_TIMER->CR1 |= TIM_CR1_CEN;
  TIO_TIMER->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN)); //disable timer
  TIO_TIMER->CNT = 0;
  TIO_TIMER->ARR = 200-1; //timer clock is running at 2kHz
  TIO_TIMER->SR = (uint16_t)~TIM_IT_Update; //clear pending bit
  TIO_TIMER->CR1 |= TIM_CR1_CEN; //enable timer
}

void selKbCol(uint16_t Col) {
  switch (Col) {
    case 0:
      KEY_C0(0);
      break;
    case 1:
      KEY_C1(0);
      break;
    case 2:
      KEY_C2(0);
      break;
    case 3:
      KEY_C3(0);
      break;
    default:
      break;
  }
}

const uint32_t KeyCodes[3][4] =
{
  {KEY_CABLE_CC, KEY_CAR_CC, KEY_UP,   KEY_ESC},
  {KEY_CABLE_S3, KEY_CAR_CP, KEY_DOWN, 0},
  {KEY_CAR_PE,   KEY_CAR_S2, KEY_RUN,  0}
};

uint16_t ReadKeys(void)
{
  uint16_t col, keys, newKey, count;
  static uint16_t LastKey;
  
  for (col=count=newKey=0; col<4; col++) {
    MASK_KEY_COLS;
    selKbCol(col);
    keys = ReadKeyRows;
    MASK_KEY_COLS;
//    keys = GPIO_ReadInputData(GPIOB);

    if (keys) {
      if (keys & 0x01){
        newKey = KeyCodes[2][col];
        count++;
      }else if (keys & 0x02){
        newKey = KeyCodes[1][col];
        count++;
      }else if (keys & 0x04){
        newKey = KeyCodes[0][col];
        count++;
      }
    }    
  }  
  if (count != 1) //multiple keys
    newKey = 0;
  if (newKey != LastKey) //same keys
    LastKey = newKey;
  else  
    newKey = 0;
  return newKey;
}

void ResetWDT(void)
{
#ifdef WDT_ENABLE
  IWDG_ReloadCounter();
#endif
}



