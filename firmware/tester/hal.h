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
 
#ifndef __HAL_H__
#define __HAL_H__
#include "stm32f10x.h"

/*******************************************************************
*
* Do not compile with optimization (use level 0). Otherwise it will 
* have many timing problems e.g. I2C, delay loop
*
*******************************************************************/

#define _DEBUG
#define USE_SWJ
//#define WDT_ENABLE

#ifndef _DEBUG
  #define WRITE_PROTECTION_ENABLE
  #define READ_PROTECTION_ENABLE
#endif

#define OS_TICK_FREQ		      1000
#define MsPerTick			        (1000/OS_TICK_FREQ)
#define MsToOSTicks(x)		    ((x)/MsPerTick)

#define LCD_BACK_LIGHT_DLY    MsToOSTicks(30000) /* 30s */
#define READ_KEY_DLY          MsToOSTicks(100)
#define MEASURE_DLY           MsToOSTicks(50)

#define ON                  1
#define OFF                 0
#define BV(x)               ((uint16_t)(1<<x))
#define CountOf(x)          (sizeof(x)/sizeof(*x))

/* Timer defines */
#define ICP_TIMER           TIM2
#define ADC_TIMER           TIM3
#define TIO_TIMER           TIM4
#define ICP_TIMER_FREQ      36000000

/* ADC defines */
#define ADC_SAMPLE_FREQ       100 /*Hz*/

/* DMA channel for ADC */
#define ADC_DMA_Channel       DMA1_Channel1

/* LCD defines */
#define LCD_BLA               BV(13)
#define LCD_CS1               BV(12)
#define LCD_RES               BV(11)
#define LCD_RS                BV(10)
#define LCD_WR                BV(9)
#define LCD_RD                BV(8)
#define SetLcdCtrlPins(x)     GPIO_SetBits(GPIOC, (uint16_t)x)
#define ClrLcdCtrlPins(x)     GPIO_ResetBits(GPIOC, (uint16_t)x)     
#define LcdBackLight(x)       GPIO_WriteBit(GPIOC, LCD_BLA, (x)?Bit_RESET:Bit_SET)
#define LoadLcdData(x)        GPIOC->ODR = (GPIOC->ODR & 0xff00)|(uint8_t)(x)

/* Relay defines */
#define CABLE_CC_CLOSE        BV(15)
#define CABLE_S3_OPEN         BV(14)
#define CAR_PE_CLOSE          BV(13)
#define CAR_CC_CLOSE          BV(12)
#define CAR_CP_CLOSE          BV(11)
#define CAR_S2_CLOSE          BV(10)
#define RelayCtrl(s, x)       GPIO_WriteBit(GPIOB, s, (x)?Bit_SET:Bit_RESET)

/* key code defines */
#define KEY_CABLE_CC          BV(0)
#define KEY_CABLE_S3          BV(1)
#define KEY_CAR_PE            BV(2)
#define KEY_CAR_CC            BV(3)
#define KEY_CAR_CP            BV(4)
#define KEY_CAR_S2            BV(5)
#define KEY_UP                BV(6)
#define KEY_DOWN              BV(7)
#define KEY_RUN               BV(8)
#define KEY_ESC               BV(9)

/* Keypad interface defines */
#define KEY_C0(x)             GPIO_WriteBit(GPIOB, GPIO_Pin_3, (BitAction)x)
#define KEY_C1(x)             GPIO_WriteBit(GPIOB, GPIO_Pin_4, (BitAction)x)
#define KEY_C2(x)             GPIO_WriteBit(GPIOB, GPIO_Pin_5, (BitAction)x)
#define KEY_C3(x)             GPIO_WriteBit(GPIOB, GPIO_Pin_6, (BitAction)x)
#define MASK_KEY_COLS         GPIO_SetBits(GPIOB, GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6)
#define ReadKeyRows           ((~GPIO_ReadInputData(GPIOB) >> 7) & 0x07)

/* circuit parameters */
#define RCC_PULLUP          470
#define VREF                3.3
#define R10A_MIN            1350
#define R10A_MAX            1650
#define R16A_MIN            612
#define R16A_MAX            748
#define R32A_MIN            198
#define R32A_MAX            240
#define R63A_MIN            90
#define R63A_MAX            110
#define RS3OPEN_MIN         3135
#define RS3OPEN_MAX         3696

extern __IO uint16_t ADCConvertedValues[20];

void TIM_Configuration(void);
void GPIO_Configuration(void);
void NVIC_Configuration(void);
void RCC_Configuration(void);
void DMA_Configuration(void);
void ADC_Configuration(void);
void IWDG_Configuration(void);
void WatchdogStart(void);
void WatchdogFeed(void);
void TimerStart(uint16_t msec);
void TimerStop(void);
uint16_t ReadKeys(void);
void ResetWDT(void);
void InitRelays(void);
void MeasurePilotSignalStart(void);

#endif

