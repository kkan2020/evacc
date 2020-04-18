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
#include <math.h>
#include "stm32f10x.h"
#include "rtx_os.h"
#include "hal.h"
#include "stm32f10x_it.h"
#include "CT_Thread.h"
#include "app_main.h"

#define ADC2AMPERE(x)        ((double)x*3.3*CTConvFactor/4095)

static volatile float CurrentCT[3];
static volatile double EnergyConsumedJS; /* Joule Second*/
static float VoltLN;
static double CTConvFactor;

static void LoadVars(void)
{
  VoltLN = Config.Private.Power.ChargeVoltage;
  CTConvFactor = Config.Private.Power.CTAmperPerVolt;
}

void ResetPowerVar(void)
{
  MASK_IRQ
    for (int i=0; i<3; i++)
      CurrentCT[i] = 0;
    EnergyConsumedJS = 0;
  UNMASK_IRQ
}
  
void GetPowerVar(float *CurrentA, float *EnergykWh)
{
  MASK_IRQ
    if (CurrentA)
      *CurrentA = CurrentCT[0]+CurrentCT[1]+CurrentCT[2];
    if (EnergykWh)
      *EnergykWh = EnergyConsumedJS/3600000L;
  UNMASK_IRQ
}

static int chargingCount=0;

void ResetChargingCounter(void)
{
  chargingCount = 0;
}

int IsDrawingCurrent(float current)
{
  #define DLY_COUNT 10
  if (current>1.0)
  {
    if (chargingCount<DLY_COUNT)
      chargingCount++;
    return chargingCount == DLY_COUNT? 1: 0;
  }
  else
  {   
    if (chargingCount>0)
      chargingCount--;
    return chargingCount==0? 0: 1;
  }
}

float GetChargingCurrents(float phases[3])
{
  float total = 0.0;
  MASK_IRQ
    for (int i=0; i<3; i++) 
    {
      phases[i] = CurrentCT[i];
      total += phases[i];
    }
  UNMASK_IRQ
  return total;
}

#ifdef EMULATION_ENABLED
void simulate_processSamples(void)
{
  static int i, n;
  static struct _rand_state rstate;
  static float totalCurrent, singlePhCurrent;
  static float phases[3];
  extern __IO uint16_t CapacityOffered;
  
  totalCurrent = phases[0]=phases[1]=phases[2]=0;
  if (IS_3PHASE_POWER)
  {
    n = 3;
    singlePhCurrent = CapacityOffered/3;
  }
  else
  {
    n =1;
    singlePhCurrent = CapacityOffered;
  }  
  _srand_r(&rstate, SysTick->VAL);
  for (i=0; i<n; i++)
  {
    if (IS_CONTACTOR_ON)
      phases[i] = singlePhCurrent+(float)_rand_r(&rstate)/0xffffffff;
    totalCurrent += phases[i];
  }
  MASK_IRQ
    EnergyConsumedJS += (totalCurrent*VoltLN*CT_SAMPLE_SIZE)/CT_SAMPLE_FREQ;
    for (i=0; i<3; i++)
      CurrentCT[i] = phases[i];
  UNMASK_IRQ
}
#else  
void processSamples(__IO int16_t AdcVales[])
{
  static double pole = 0.996;
  static uint16_t i, j, n;
  static __IO int16_t *x;
  static int16_t y;
  static int32_t Acc[3], Prev_x[3], Prev_y[3], sum;
  static int32_t A, t; 
  static float totalCurrent;
  static float phases[3];
  
  A = (int32_t)(32768.0*(1.0 - pole));
  totalCurrent = 0;
  for (i=0; i<3; i++)
  {
    //DC blocking filter
    x = &AdcVales[i];
    sum = 0;
    for (j=n=0; j<CT_SAMPLE_SIZE; j++, n+=3)
    {
      Acc[i]   -= Prev_x[i];
      Prev_x[i] = (int32_t)x[n]<<15;
      Acc[i]   += Prev_x[i];
      Acc[i]   -= A*Prev_y[i];
      Prev_y[i] = Acc[i]>>15; // quantization happens here
      y   = (int16_t)Prev_y[i];
      // Acc has y[n] in upper 17 bits and -e[n] in lower 15 bits
      sum += y*y;
    }
    //root mean square and trancate to 1 decimal point
    t = ADC2AMPERE(sqrt((double)sum/CT_SAMPLE_SIZE))*10; 
    phases[i] = (float)t/10;
    totalCurrent += phases[i];
  }

  MASK_IRQ
  EnergyConsumedJS += (totalCurrent*VoltLN*CT_SAMPLE_SIZE)/CT_SAMPLE_FREQ;
    for (i=0; i<3; i++)
      CurrentCT[i] = phases[i];
  UNMASK_IRQ
}
#endif

osRtxThread_t CTThread_tcb;
uint64_t CT_ThreadStk[128];
const osThreadAttr_t CTThread_attr = { 
  .cb_mem = &CTThread_tcb,
  .cb_size = sizeof(CTThread_tcb),
  .stack_mem = CT_ThreadStk,  
  .stack_size = sizeof(CT_ThreadStk),
  .priority = osPriorityRealtime,
};

void CTThread(void *arg)
{
  static int32_t flags;
  static __IO int16_t *data;
  
  TIM_Cmd(ADC1_TIMER, ENABLE); //start ADC sampling timer 
  while(1)
  { 
    flags = osThreadFlagsWait(CT_HALF_TRANS|CT_TRANS_COMP, osFlagsWaitAny, osWaitForever); 
    if (flags & CT_HALF_TRANS) 
    {
      data = &CT_ADCVales[0];
      osThreadFlagsClear(CT_HALF_TRANS);
    }
    else if (flags & CT_TRANS_COMP)//(flag & 0x02)
    {
      data = &CT_ADCVales[CT_ADC_BUFFER_SIZE/2];    
      osThreadFlagsClear(CT_TRANS_COMP);
    }
    
#ifdef EMULATION_ENABLED
    simulate_processSamples();
#else
    processSamples(data); 
#endif
    
    flags = osThreadFlagsGet();
    if (flags & CT_UPD_VARS)
    {
      MASK_IRQ
        LoadVars();
      UNMASK_IRQ
      osThreadFlagsClear(CT_UPD_VARS);
    }
  }
}
  


