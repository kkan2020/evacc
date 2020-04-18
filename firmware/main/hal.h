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

#define FIXED_CABLE
#define _DEBUG
#define USE_SWJ
//#define WDT_ENABLE
//#define REGEN_UIDS
//#define REFRESH_JWTS
//#define REFRESH_SER_URL
#define EMULATION_ENABLED

#ifndef _DEBUG
  #define WRITE_PROTECTION_ENABLE
  #define READ_PROTECTION_ENABLE
#endif

#define OS_TICK_FREQ		      1000
#define MsPerTick			        (1000/OS_TICK_FREQ)
#define MsToOSTicks(x)		    ((x)/MsPerTick)

#define ON                  1
#define OFF                 0
#define TRUE                1
#define FALSE               0

#define BV(x)               ((uint16_t)(1<<x))
#define CountOf(x)          (sizeof(x)/sizeof(*x))
#define MIN(a, b)           (a>b)?b:a
#define MASK_IRQ            int masked = __disable_irq();
#define UNMASK_IRQ          if (!masked) __enable_irq(); 

/* Default parameters */
#define DEF_CHARGE_CURRENT                  8 /* A */
#define DEF_CHARGE_VOLTAGE                220 /* VAC */
#define DEF_WIFI_MODE                       1
#define DEF_WIFI_SSID                     "eturtle"

/* Charger parameters **************************************************************/
#define EV_NO_S2_CURRENT_MAX                     8 /* A */
#define CHARGE_CURRENT_MIN                       6 /* A */
#define CHARGE_CURRENT_MAX                      63 /* A */
#define IS_3PHASE_POWER                          0
#define CHARGE_VOLTAGE_MIN                      85 /* V */
#define CHARGE_VOLTAGE_MAX                     250 /* V */
#define POWER_MANAGEMENT                         0 /* managed power*/
#define CT_AMPERE_PER_VOLT                      50 /* A/V */
#define WORKING_TEMP_MIN                       -22 /* C */
#define WORKING_TEMP_MAX                        52 /* C */
#define TEMP_HYSTERESIS                          2 /* C */

#if (IS_3PHASE_POWER!=0)
  #define SINGLE_PH_CURRENT_MAX                 (Config.Private.Power.ChargeCurrentMax/3)
#else
  #define SINGLE_PH_CURRENT_MAX                 Config.Private.Power.ChargeCurrentMax
#endif
/**********************************************************************************/

/* Timing constants */
#define LCD_BACK_LIGHT_DLY    MsToOSTicks(30000) /* 30s */
#define PILOT_STATE_DLY       MsToOSTicks(50)
#define UI_UPDATE_DLY         MsToOSTicks(250)

/* Timer */
#define PWM_TIMER           TIM1 /* cannot use other timer*/
#define ADC1_TIMER          TIM3 /* cannot use other timer*/
#define I2C2_TIMER          TIM4
#define US_TIMER            TIM2
#define WIFI_UART_RX_TIM    TIM6
#define GPRS_UART_TIMER     TIM7

#define ENABLE_TIMER(x)      (x)->CR1 |= TIM_CR1_CEN
#define IS_TIMER_ENABLED(x)  ((x)->CR1 & TIM_CR1_CEN)
#define DISABLE_TIMER(x)     (x)->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN))
#define IS_NOT_TIMEOUT(x)    (((x)->SR & TIM_FLAG_Update) == (uint16_t)RESET)


/* I2C */
#define I2C_CLK_SPEED       100000
#define I2C_OWN_ADDR7       (1<<1)
#define I2C2_SLAVE_ADDR7	  0xA0

/* PWM */
#define ENABLE_PWM            PWM_TIMER->CR1 |= TIM_CR1_CEN; 
#define DISABLE_PWM           PWM_TIMER->CR1 &= (uint16_t)~TIM_CR1_CEN;
#define PWM_FREQ              1000 /* 1000Hz */

/* ADC */
#define ADC_CHNL_CT1          ADC_Channel_10
#define ADC_CHNL_CT2          ADC_Channel_11
#define ADC_CHNL_CT3          ADC_Channel_12
#define ADC_CHNL_PILOT        ADC_Channel_7
#define CT_SAMPLE_FREQ        1000
#define CT_SAMPLE_SIZE        250
#define CT_ADC_BUFFER_SIZE    (3*CT_SAMPLE_SIZE*2)

/* 1-Wired bus */
#define OneWireBus(x)         GPIO_WriteBit(GPIOC, GPIO_Pin_7, x?Bit_SET:Bit_RESET)
#define ReadOneWireBus        GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_7)

/* UARTS */
#define RFID_UART							USART2
#define WIFI_UART							UART4					
#define GPRS_UART							USART1					

/* LED */
#define LED_READY_BIT         0x01
#define LED_CHARGE_BIT        0x02
#define LED_FAULT_BIT         0x04

#define LED_SET_READY(x)      GPIO_WriteBit(GPIOC, GPIO_Pin_5, (x)?Bit_RESET:Bit_SET)
#define LED_TOGGLE_READY      GPIO_WriteBit(GPIOC, GPIO_Pin_5, GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_5)?Bit_RESET:Bit_SET)

#define LED_SET_CHARGE(x)     GPIO_WriteBit(GPIOC, GPIO_Pin_4, (x)?Bit_RESET:Bit_SET)
#define LED_TOGGLE_CHARGE     GPIO_WriteBit(GPIOC, GPIO_Pin_4, GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_4)?Bit_RESET:Bit_SET)

#define LED_SET_FAULT(x)      GPIO_WriteBit(GPIOA, GPIO_Pin_4, (x)?Bit_RESET:Bit_SET)
#define LED_TOGGLE_FAULT      GPIO_WriteBit(GPIOA, GPIO_Pin_4, GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_4)?Bit_RESET:Bit_SET)

/* LCD */
//#define LcdRD_Pin(x)          GPIO_WriteBit(GPIO?, GPIO_Pin_?, (x)?Bit_SET:Bit_RESET)         
#define LcdWR_Pin(x)          GPIO_WriteBit(GPIOB, GPIO_Pin_8, (x)?Bit_SET:Bit_RESET)         
#define LcdRS_Pin(x)          GPIO_WriteBit(GPIOB, GPIO_Pin_9, (x)?Bit_SET:Bit_RESET)         
#define LcdRES_Pin(x)         GPIO_WriteBit(GPIOC, GPIO_Pin_12, (x)?Bit_SET:Bit_RESET)         
#define LcdBL_Pin(x)          GPIO_WriteBit(GPIOD, GPIO_Pin_2, (x)?Bit_SET:Bit_RESET)         
#define LcdCS_Pin(x)          GPIO_WriteBit(GPIOC, GPIO_Pin_13, (x)?Bit_SET:Bit_RESET)  

#define LCD_CS1               BV(4)
#define LCD_RES               BV(3)
#define LCD_RS                BV(2)
#define LCD_WR                BV(1)
#define LCD_RD                BV(0)

#define LcdBackLight(x)       LcdBL_Pin(!x)
#define LoadLcdData(x)        GPIOB->BSRR = (x)&0xff;GPIOB->BRR = (~x)&0xff;

/* cover detect */
#define IR_PULSE_TX(x)        GPIO_WriteBit(GPIOC, GPIO_Pin_15, x?Bit_SET:Bit_RESET)
#define IR_PULSE_RX           GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_14)

/* emmergency stop switch */

#ifdef EMULATION_ENABLED
#define IS_EM_STOP_PRESSED    (!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9))
#else
#define IS_EM_STOP_PRESSED    (!GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9) && !IS_DEBUG_MODE)
#endif

/* Contactor */
#define CONTACTOR(x)          GPIO_WriteBit(GPIOC, GPIO_Pin_8, x?Bit_RESET:Bit_SET)
#define IS_CONTACTOR_ON       !GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_8)

#ifdef FIXED_CABLE
  #define IS_CABLE_CONNECTED    1 
//#else 
//  #define IS_CABLE_CONNECTED    (!GPIO_ReadInputDataBit(GPIO?, GPIO_Pin_?))
#endif

/* EEPROM */
#define EEPWriteProtect(x)    GPIO_WriteBit(GPIOB, GPIO_Pin_12, x?Bit_SET:Bit_RESET)

/* Buzzor */
#define Buzzor(x)             GPIO_WriteBit(GPIOA, GPIO_Pin_15, x?Bit_SET:Bit_RESET)

/* SOCKET 1 (WIFI) */
#define SO1_RESET(x)          GPIO_WriteBit(GPIOC, GPIO_Pin_6, x?Bit_SET:Bit_RESET)


/* SOCKET 2 (GPRS) */
#define SO2_EN(x)             GPIO_WriteBit(GPIOC, GPIO_Pin_3, x?Bit_SET:Bit_RESET)
#define SO2_SLEEP(x)          GPIO_WriteBit(GPIOA, GPIO_Pin_6, x?Bit_SET:Bit_RESET)
#define SO2_RESET(x)          GPIO_WriteBit(GPIOA, GPIO_Pin_5, x?Bit_SET:Bit_RESET)

/* Hardware configuration */
#define IS_DEBUG_MODE         (!GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1))


extern __IO int16_t CT_ADCVales[CT_ADC_BUFFER_SIZE];


void TIM_Configuration(void);
void GPIO_Configuration(void);
void PWM_Configuration(void);
void NVIC_Configuration(void);
void RCC_Configuration(void);
void DMA_Configuration(void);
void ADC_Configuration(void);
void I2C_Configuration(void);
void USART_Configuration(void);
void IWDG_Configuration(void);
void WatchdogFeed(void);
void ResetWDT(void);
float PWM_GetDutyCycle(void);
int PWM_IsDC(void);
void PWM_SetDutyCycle(float DutyCycle);
void PWM_ExtTriggerDisable(void);
void PWM_ExtTriggerEnable(void);
void SetTimeout_us(TIM_TypeDef* TIMx, uint16_t us);
void GPRS_Configuration(void);
void WiFi_Reset(void);
void LedSetState(int mask, int state);
void LedToggleState(int mask);
void SetLcdCtrlPins(int x);
void ClrLcdCtrlPins(int x) ;


#endif

