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
#include <stdio.h>
#include "RTE_Components.h"
#include  CMSIS_device_header
#include "cmsis_os2.h"
//include rtx_os.h for types of RTX objects
#include "rtx_os.h"
#include "hal.h"
#include "stm32f10x_it.h"
#include "lcg-par.h"
#include "utils.h"

__IO uint8_t CableCC_Close, CableS3_Open, CarPE_Close, CarCC_Close, CarCP_Close, CarS2_Close, ForceCarS2_Close;
__IO uint8_t CableS3Closed;
__IO uint16_t CableCapacity, ChargerCapacity;

uint8_t IsCableConnected(void)
{
  return CableCapacity>0;
}

uint8_t IsChargingAllowed(void)
{
  return CableS3Closed && CableCapacity && ChargerCapacity;
}

uint16_t ChargingCapacity(void)
{
  if (!IsChargingAllowed())
    return 0;
  if (ChargerCapacity > CableCapacity)
    return CableCapacity;
  else 
    return ChargerCapacity;
}

void updateStates(void) {
//  CarS2_Close = IsChargingAllowed();

  RelayCtrl(CABLE_CC_CLOSE, CableCC_Close);    
  RelayCtrl(CABLE_S3_OPEN, CableS3_Open);
  RelayCtrl(CAR_PE_CLOSE, CarPE_Close);
  RelayCtrl(CAR_CC_CLOSE, CarCC_Close);
  RelayCtrl(CAR_CP_CLOSE, CarCP_Close);
  RelayCtrl(CAR_S2_CLOSE, CarS2_Close||ForceCarS2_Close);      
}

char* writeInteger(char *line, char* caption, int16_t value, char* unit)
{
  sprintf(line, "%s%d%s", caption, value, unit);
  return line;
}

char* writeFloat(char *line, char* caption, float value, char* unit)
{
  sprintf(line, "%s%.2f%s", caption, value, unit);
  return line;
}

char* writeBool(char *line, char* caption, int8_t value)
{
  sprintf(line, "%s%s", caption, value? "YES":"NO");
  return line;
}

void updateScreen(void)
{
  TRect r;
  char line[40];
  r = Windows[0].WndRect;
  ClearRect(0, r, Windows[0].Backgnd, cpClipToParent);
  r.Height = DefFontHeight() + 1;
    
  writeBool(line, "Cable locked:", CableS3Closed);
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);

  r.TopLeft.Pt.Row += r.Height;  
  writeInteger(line, "Cable cap:", CableCapacity, "A");
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);
  
  r.TopLeft.Pt.Row += r.Height;  
  writeFloat(line, "PWM:", PWM_DutyCycle, "%");
  writeInteger(&line[strlen(line)], " ", PWM_Freq, "Hz");
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);

  r.TopLeft.Pt.Row += r.Height;  
  writeInteger(line, "Charger cap:", ChargerCapacity, "A");
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);
  
  r.TopLeft.Pt.Row += r.Height;  
  writeInteger(line, "Charging cap:", ChargingCapacity(), "A");
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);
  
  r.TopLeft.Pt.Row += r.Height;  
  writeBool(line, "Ready to charge:", IsChargingAllowed());
  OutText(0, r, line, 0, TEXT_WORD_WRAP, cpClipToClient);

  RenderScreen();
}

void UIThread(void *arg) 
{
  InitLcg();
  SetLcgContrast(26);
  CreateWnd(-1, 0, MakeRect(0, 0, SCREEN_ROWS, SCREEN_COLS), 0, &ScreenCanvas, borNone, bkNormal);
  LcdBackLight(ON);  
  while(1) 
  {
    uint16_t key = ReadKeys();
    switch(key) 
    {
      case KEY_CABLE_CC:
        CableCC_Close = !CableCC_Close;
        break;
      case KEY_CABLE_S3:
        CableS3_Open = !CableS3_Open;
        break;
      case KEY_CAR_PE:
        CarPE_Close = !CarPE_Close;
        break;
      case KEY_CAR_CC:
        CarCC_Close = !CarCC_Close;
        break;
      case KEY_CAR_CP:
        CarCP_Close = !CarCP_Close;
        break;
      case KEY_CAR_S2:
        ForceCarS2_Close = !ForceCarS2_Close;
        break;
      case KEY_UP:
        break;
      case KEY_DOWN:
        break;
      case KEY_RUN: 
        break;
      case KEY_ESC:
        break;
      default:
        break;
    }   
    updateScreen();
    osDelay(READ_KEY_DLY);    
  }  
}

void MeasureThread(void *arg)
{
  MeasurePilotSignalStart();
  while(1)
  {            
    //update Cable capacity
    if ((R10A_MIN<CCResistance)&&(CCResistance<R10A_MAX))
    {
      CableCapacity = 10;
      CableS3Closed = 1;
    }
    else if ((R16A_MIN<CCResistance)&&(CCResistance<R16A_MAX))
    {
      CableCapacity = 16;
      CableS3Closed = 1;
    }
    else if ((R32A_MIN<CCResistance)&&(CCResistance<R32A_MAX))
    {
      CableCapacity = 32;
      CableS3Closed = 1;
    }
    else if ((R63A_MIN<CCResistance)&&(CCResistance<R63A_MAX))
    {
      CableCapacity = 32;
      CableS3Closed = 1;
    }
//    else if ((RS3OPEN_MIN<=CCResistance)&&(CCResistance<=RS3OPEN_MAX)) //cable S3 opened
//      CableS3Closed = 0;
    else //cable removed
      CableS3Closed = CableCapacity = 0;
    
    //update Charger capacity
    if ((970<=PWM_Freq)&&(PWM_Freq<=1030))
    {
      if (PWM_DutyCycle<3)
        ChargerCapacity = 0;//charging forbidded
      else if ((3<=PWM_DutyCycle)&&(PWM_DutyCycle<=7))
        ChargerCapacity = 0;//need digital interface
      else if ((7<PWM_DutyCycle)&&(PWM_DutyCycle<8))
        ChargerCapacity = 0;//charging forbidded
      else if ((8<=PWM_DutyCycle)&&(PWM_DutyCycle<10))
        ChargerCapacity = 6;
      else if ((10<=PWM_DutyCycle)&&(PWM_DutyCycle<=85))
        ChargerCapacity = PWM_DutyCycle*0.6+0.5;
      else if ((85<PWM_DutyCycle)&&(PWM_DutyCycle<=90))
      {
        ChargerCapacity = (PWM_DutyCycle-64)*2.5+0.5;
        if (ChargerCapacity>63)
          ChargerCapacity = 63;
      }
      else if ((90<PWM_DutyCycle)&&(PWM_DutyCycle<=97))
        ChargerCapacity = 0; //reserved
      else if (PWM_DutyCycle>97)
        ChargerCapacity = 0;//charging forbidded
    }
    else
      ChargerCapacity = 0;
    
    updateStates();
    osDelay(MEASURE_DLY);    
  }
}

osRtxThread_t UIThread_tcb;
uint64_t UIThreadStk[256]; 
const osThreadAttr_t UIThread_attr = { 
  .cb_mem = &UIThread_tcb,
  .cb_size = sizeof(UIThread_tcb),
  .stack_mem = UIThreadStk,  
  .stack_size = sizeof(UIThreadStk),
  .priority = osPriorityNormal,
};

osRtxThread_t MeasureThread_tcb;
uint64_t MeasureThreadStk[256];
const osThreadAttr_t MeasureThread_attr = { 
  .cb_mem = &MeasureThread_tcb,
  .cb_size = sizeof(MeasureThread_tcb),
  .stack_mem = MeasureThreadStk,  
  .stack_size = sizeof(MeasureThreadStk),
  .priority = osPriorityAboveNormal,
};

// Define ID object for thread
osThreadId_t UIThread_id, MeasureThread_id; 

/*----------------------------------------------------------------------------
 * Application main thread
 *---------------------------------------------------------------------------*/
void app_main (void *argument) {
  uint32_t param = NULL;
 
  UIThread_id = osThreadNew(UIThread, &param, &UIThread_attr); 
  MeasureThread_id = osThreadNew(MeasureThread, &param, &MeasureThread_attr); 
  while(1);
}

int main(void) 
{
  uint32_t i;
  
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

  ResetWDT();  
  //stop timer2, timer3 and IDWG on CPU halt
  DBGMCU->CR |= DBGMCU_CR_DBG_TIM2_STOP|DBGMCU_CR_DBG_TIM3_STOP|DBGMCU_CR_DBG_TIM4_STOP|DBGMCU_CR_DBG_IWDG_STOP; 
  //wait for power supply to stablize
  for (i=0; i<10000L; i++) __nop();  
  ResetWDT();  
  RCC_Configuration(); 
  GPIO_Configuration();
  NVIC_Configuration();
  TIM_Configuration();  
  DMA_Configuration();
  ADC_Configuration();
  LcdBackLight(OFF);
  ResetWDT();  
  TIM_Cmd(ADC_TIMER, ENABLE); //start sampling CC signal

  SystemCoreClockUpdate();
  osKernelInitialize();                 // Initialize CMSIS-RTOS
  osThreadNew(app_main, NULL, NULL);    // Create application main thread
  osKernelStart();                      // Start thread execution
  while(1);
}
