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
 
#include <stdlib.h>
#include <string.h> 
#include "RTE_Components.h"
#include  CMSIS_device_header
#include "cmsis_os2.h"
//include rtx_os.h for types of RTX objects
#include "rtx_os.h"
#include "hal.h"
#include "stm32f10x_it.h"
#include "lcg-par.h"
#include "i2c_hw.h"
#include "app_main.h"
#include "clock_calendar.h"

osRtxThread_t AppMainThread_tcb;
uint64_t AppMainThreadStk[64]; 
const osThreadAttr_t AppMainThread_attr = { 
  .cb_mem = &AppMainThread_tcb,
  .cb_size = sizeof(AppMainThread_tcb),
  .stack_mem = AppMainThreadStk,  
  .stack_size = sizeof(AppMainThreadStk),
  .priority = osPriorityAboveNormal,
};

int main(void) 
{  
#ifdef WRITE_PROTECTION_ENABLE
  uint32_t WRPR_Value;
#endif  
  
#ifndef _DEBUG  
  IWDG_Configuration();
#endif
  
  FLASH_Unlock();
#ifdef READ_PROTECTION_ENABLE
  if ( FLASH_GetReadOutProtectionStatus() != SET )
  {
    FLASH_Unlock();
    ResetWDT();  
    FLASH_ReadOutProtection(ENABLE);
    NVIC_SystemReset();
  }
#endif
   
#ifdef WRITE_PROTECTION_ENABLE
  /* Get pages write protection status */
  WRPR_Value = FLASH_GetWriteProtectionOptionByte();
  
  /* Check if desired pages are not yet write protected */
  if(~WRPR_Value != FLASH_WRProt_AllPages)
  {
    /* Erase all the option Bytes because if a program operation is 
      performed on a protected page, the Flash memory returns a 
      protection error */
    FLASH_EraseOptionBytes();
    ResetWDT();  
    /* Enable the pages write protection */
    FLASH_EnableWriteProtection(FLASH_WRProt_AllPages);
    /* Generate System Reset to load the new option byte values */
    NVIC_SystemReset();
  }
#endif
  FLASH_Lock();
  
#ifdef _DEBUG
  //stop timer1, timer3, timer4 and IDWG on CPU halt
//  DBGMCU->CR |= DBGMCU_CR_DBG_TIM1_STOP|DBGMCU_CR_DBG_TIM3_STOP|DBGMCU_CR_DBG_TIM4_STOP|DBGMCU_CR_DBG_IWDG_STOP; 
  DBGMCU->CR |= DBGMCU_CR_DBG_TIM3_STOP|DBGMCU_CR_DBG_TIM4_STOP|DBGMCU_CR_DBG_IWDG_STOP; 
#endif
  
  ResetWDT();  
  RCC_Configuration(); 
  GPIO_Configuration();
  CONTACTOR(OFF);
  EEPWriteProtect(ON);
  Buzzor(OFF);
  NVIC_Configuration();
  TIM_Configuration();  
  ADC_Configuration();
  DMA_Configuration();
  I2C_Configuration();
  USART_Configuration();
  RTC_Configuration();
  PWM_Configuration();
  ResetWDT();   
  
  SystemCoreClockUpdate();
  osKernelInitialize();                 // Initialize CMSIS-RTOS
  osThreadNew(app_main, NULL, &AppMainThread_attr);    // Create application main thread
  osKernelStart();                      // Start thread execution
  while(1);
}
