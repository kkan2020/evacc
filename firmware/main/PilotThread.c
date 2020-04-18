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
#include "stm32f10x.h"
#include "cmsis_os2.h"
#include "rtx_os.h"
#include "hal.h"
#include "CT_Thread.h"
#include "app_main.h"
#include "RFIDThread.h"
#include "Trans.h"
#include "stm32f10x_it.h"
#include "PilotThread.h"

#define DELTA_V                     0.8 /* v */
#define REQUEST_MAX_CAPACITY        0
#define VOLTAGE_CHANGE_DLY            MsToOSTicks(20)

/* ADC JEXTTRIG mask */
#define CR2_JEXTTRIG_Set            ((uint32_t)0x00008000)
#define CR2_JEXTTRIG_Reset          ((uint32_t)0xFFFF7FFF)

osRtxThread_t PilotThread_tcb;
uint64_t PilotThreadStk[64];
const osThreadAttr_t PilotThread_attr = { 
  .cb_mem = &PilotThread_tcb,
  .cb_size = sizeof(PilotThread_tcb),
  .stack_mem = PilotThreadStk,  
  .stack_size = sizeof(PilotThreadStk),
  .priority = osPriorityHigh,
};

osTimerId_t PWM_timer, S9Vdc_timer, S6Vdc_timer, SStop_timer, Solenoid_timer, OverCurrent_timer;
uint16_t S6VdcPluseCompleted, EVWithoutS2;
uint32_t PWM_timeouted, S9Vdc_timeouted, S6Vdc_timeouted, SStop_timeouted;
uint32_t Solenoid_timeouted, OverCurrent_timeouted;

static uint64_t Pluse6VdcBegin, Pluse6VdcEnd, PluseWidth;
static uint16_t ChargeCurrentMax;
static TVoltageObj* CurState=&S12Vdc;
static TTransState TrState;
__IO uint16_t CapacityReserved, CapacityOffered;

void LoadCapacityVars(void)
{
  MASK_IRQ
    ChargeCurrentMax = Config.Private.Power.ChargeCurrentMax;
    if (ChargeCurrentMax>CHARGE_CURRENT_MAX)
      ChargeCurrentMax = CHARGE_CURRENT_MAX; 
  UNMASK_IRQ
}

void SolenoidOpen(void)
{
}

void SolenoidClose(void)
{
}

void ReleaseCapacity(void)
{
  if (CapacityReserved>0)
  {
    if (IS_POWER_MANAGED)
    {
      //send capacity booking (release)
      //wait for reply
    }  
    CapacityOffered = CapacityReserved = 0;
  }
}

uint16_t AcquireCapacity(uint16_t Request)
{
  if (Request==REQUEST_MAX_CAPACITY)
    Request = ChargeCurrentMax;  

  if (IS_POWER_MANAGED)
  {
    if (CapacityReserved!=Request)
    {
    //send capacity booking (reserve)
    //wait for reply
    //CapacityReserved = ReserveCapacity(Request)
    }
    Request = CapacityReserved>Request? Request: CapacityReserved;
  }
  else
    CapacityReserved = Request;
  return Request;
}

uint16_t GetCapacityOffer(uint16_t Request)
{
  if (!IsHardwareOkay() || !IsDlyTimerStopped() || IsTempOutOfRange())
    return 0;
  if (EVWithoutS2 && (Request > EV_NO_S2_CURRENT_MAX))
    Request = EV_NO_S2_CURRENT_MAX;
  if (Request < CHARGE_CURRENT_MIN)
    Request = 0;
  return AcquireCapacity(Request);
}

float GetDutyCycle(uint16_t Current)
{
//  if (DigitalIfNeeded)
//    return 5/100;
  if (Current==0)
    return 1;
  float cur = Current;
  float r = cur/100/0.6;
  if ((0.1<=r)&&(r<=0.85))
    return r;
  r = (cur/2.5+64)/100;
  if ((0.85<r)&&(r<=0.9))
    return r;
  return 1; //charger not available
}

float adc2Volt(uint16_t ADC_Val)
{
  float v = ADC_Val; 
  v = v*3.3/4095;
  return (v-1.7434)*9.6525 + 0.12;//add 0.12V to compensate sapling time error 
}

TPWMLevel pwmVolt2PwmLevel(float pwmV)
{
  if (((12-DELTA_V)<=pwmV)&&(pwmV<=(12+DELTA_V)))
    return pwm12V;
  if (((9-DELTA_V)<=pwmV)&&(pwmV<=(9+DELTA_V)))
    return pwm9V;
  if (((6-DELTA_V)<=pwmV)&&(pwmV<=(6+DELTA_V)))
    return pwm6V;  
  if (((-12-DELTA_V)<=pwmV)&&(pwmV<=(-12+DELTA_V)))
    return pwmN12V;
  return pwmError;
}

uint16_t getADCValueAveraged(void)
{
  uint16_t i, adcVal;
  
  adcVal = 0;
  for (i=0; i<CountOf(ADC_InjectedConvertedValueTab); i++)
    adcVal += ADC_InjectedConvertedValueTab[i];
  return adcVal/CountOf(ADC_InjectedConvertedValueTab);
}

TPWMLevel getPWMLevel(void)
{
  uint16_t adcVal;
  float pwmV;

  adcVal = getADCValueAveraged();
  pwmV = adc2Volt(adcVal);
  return pwmVolt2PwmLevel(pwmV);
}

TVoltageLevel PWM_MeasureStart(TMeasurement measurement)
{
  osStatus_t  status;
  TPWMLevel pwmLevel;
  TVoltageLevel ret;

  status = osTimerStart(PWM_timer, MsToOSTicks(100));
  PWM_timeouted = 0;  
  if (status != osOK)  
  {
    ret = piTimerError;      
    goto _exit;
  }
  InjectedGroupIndex = 0;    
  if (measurement == mmDC)
  {
    TIM_SelectOutputTrigger(PWM_TIMER, TIM_TRGOSource_Update);   
    PWM_ExtTriggerEnable();
    while ((ADC1->CR2 & CR2_JEXTTRIG_Set) && (!PWM_timeouted)) //wait until completed or timeout
      osThreadYield();
    PWM_ExtTriggerDisable();

    if (PWM_timeouted)
    {
      ret = piTimeouted;    
      goto _exit;
    }
    pwmLevel = getPWMLevel();
    switch(pwmLevel)
    {
      case pwm12V:
        ret = pi12VDC;
        break;
      case pwm9V:
        ret = pi9VDC;
        break;
      case pwm6V:
        ret = pi6VDC;
        break;
      case pwmN12V:
        ret = piN12VDC;
        break;
      default: 
        ret = piError;
        break;
    } 
  }
  else
  {
    if (measurement == mmPeak)
      TIM_SelectOutputTrigger(PWM_TIMER, TIM_TRGOSource_Update);   
    else //if (measurement == mmCrest)
      TIM_SelectOutputTrigger(PWM_TIMER, TIM_TRGOSource_OC1);   
    PWM_ExtTriggerEnable();
    while ((ADC1->CR2 & CR2_JEXTTRIG_Set) && (!PWM_timeouted)) //wait until completed or timeout
      osThreadYield();
    PWM_ExtTriggerDisable();
    if (PWM_timeouted)
    {
      ret = piTimeouted;
      goto _exit;
    }
    pwmLevel = getPWMLevel();
    switch(pwmLevel)
    {
      case pwm12V:
        ret = pi12VAC;
        break;
      case pwm9V:
        ret = pi9VAC;
        break;
      case pwm6V:
        ret = pi6VAC;
        break;
      case pwmN12V:
        ret = piN12VAC;
        break;
      default: 
        ret = piError;
        break;
    }
  }
  _exit:  
  osTimerStop(PWM_timer); 
  return ret;
}  

#ifdef EMULATION_ENABLED
static TVoltageLevel pilotState = pi12VDC;
  
void SetVoltage(TVoltageLevel state)
{
  MASK_IRQ
    pilotState = state;
  UNMASK_IRQ
}
  
TVoltageLevel GetVoltage(void)
{
  MASK_IRQ
    TVoltageLevel state = pilotState;
    if (PWM_IsDC()) //if stopped, there should have no AC signal 
    {
      switch(state)
      {
        case pi6VAC:
          state = pi6VDC;
          break;
        case pi9VAC:
          state = pi9VDC;
          break;
        case pi12VAC:
          state = pi12VDC;
          break;
        default:
          break;
      }      
    }
    else
    {
      switch(state)
      {
        case pi6VDC:
          state = pi6VAC;
          break;
        case pi9VDC:
          state = pi9VAC;
          break;
        case pi12VDC:
          state = pi12VAC;
          break;
        default:
          break;
      }      
    }
  UNMASK_IRQ
  return state;
}
#else
TVoltageLevel GetVoltage(void)
{
  TVoltageLevel piState;
  int errorCount = 0;
  
  while(1)
  {
    if (PWM_IsDC())
      piState = PWM_MeasureStart(mmDC);
    else 
    {
      piState = PWM_MeasureStart(mmCrest); //detect diode
      if (piState == piTimeouted) //no PWM signal
        return piError;
      if ((piState == piN12VDC)||(piState == piN12VAC)) //diode exists
      {
        piState = PWM_MeasureStart(mmPeak);
        if (piState == piTimeouted) //no PWM signal
          return piError;
      }
      else
        piState = piError;
    }
    if (piState == piError)
    {
      if (errorCount<9)
        errorCount++;
      if (errorCount==9)
        return piError;
    }
    else 
      return piState;
  }
}
#endif



void S12VdcAction(TVoltageObj* Self);
void S9VdcAction(TVoltageObj* Self);
void S6VdcAction(TVoltageObj* Self);
void S6VacAction(TVoltageObj* Self);
void S9VacAction(TVoltageObj* Self);
void S12VacAction(TVoltageObj* Self);
void SErrorAction(TVoltageObj* Self);
void SStopAction(TVoltageObj* Self);
void SPanicAction(TVoltageObj* Self);
void SDigitalIfAction(TVoltageObj* Self);

TVoltageObj* S12VdcNext(TVoltageObj* Self);
TVoltageObj* S9VdcNext(TVoltageObj* Self);
TVoltageObj* S6VdcNext(TVoltageObj* Self);
TVoltageObj* S6VacNext(TVoltageObj* Self);
TVoltageObj* S9VacNext(TVoltageObj* Self);
TVoltageObj* S12VacNext(TVoltageObj* Self);
TVoltageObj* SErrorNext(TVoltageObj* Self);
TVoltageObj* SStopNext(TVoltageObj* Self);
TVoltageObj* SPanicNext(TVoltageObj* Self);
TVoltageObj* SDigitalIfNext(TVoltageObj* Self);

TVoltageObj S12Vdc = {"S12Vdc", 0, S12VdcAction, S12VdcNext};
TVoltageObj S9Vdc = {"S9Vdc", 0, S9VdcAction, S9VdcNext};
TVoltageObj S6Vdc = {"S6Vdc", 0, S6VdcAction, S6VdcNext};
TVoltageObj S9Vac = {"S9Vac", 0, S9VacAction, S9VacNext};
TVoltageObj S6Vac = {"S6Vac", 0, S6VacAction, S6VacNext};
TVoltageObj S12Vac = {"S12Vac", 0, S12VacAction, S12VacNext};
TVoltageObj SError = {"Error", 0, SErrorAction, SErrorNext};
TVoltageObj SStop = {"Stopped", 0, SStopAction, SStopNext};
TVoltageObj SPanic = {"Panic", 0, SPanicAction, SPanicNext};
TVoltageObj SDigitalIf = {"DigitalIf", 0, SDigitalIfAction, SDigitalIfNext};

void EnergyOutput(uint16_t OnOff)
{
  if (OnOff==OFF)
  {
    CONTACTOR(OFF);
    ReleaseCapacity();
  }
  else //ON
  {
   if ((CapacityReserved>0) && (TrState==trCharging))
      CONTACTOR(ON);
  }
}

void S12VdcAction(TVoltageObj* Self)
{
  PWM_SetDutyCycle(1); //PWM output 12VDC
  EnergyOutput(OFF);  
  osDelay(VOLTAGE_CHANGE_DLY); //wait for stable   
  SolenoidOpen();
}

TVoltageObj* S12VdcNext(TVoltageObj* Self)
{
  TVoltageLevel piState;
  TVoltageObj* next;
  
	if (IS_PANIC) 
		return &SPanic;	
  piState = GetVoltage();
  switch (piState)
  {
    case pi12VDC:
      Trans_SetState(trIdle, 0);
      next = &S12Vdc;
      break;
    case pi9VDC:
      EVWithoutS2 = 0;
      Trans_SetState(trHandshake, 0);
      if (Self->Prev!=Self)
        SolenoidClose();
      next = &S9Vdc;
      break;
    case pi6VDC:
      EVWithoutS2 = 1;
      Trans_SetState(trHandshake, 0);
      if (Self->Prev!=Self)
        SolenoidClose();
      next = &S6Vdc;
      break;
    default:
      next = &SError;
      break;
  }
  next->Prev = Self; 
  return next;
}

void S9VdcAction(TVoltageObj* Self)
{
  PWM_SetDutyCycle(1); //PWM output 12VDC
  EnergyOutput(OFF);
  osDelay(VOLTAGE_CHANGE_DLY); //wait for voltage stable     
  if (Self->Prev != Self) //state transition
  {
    osTimerStart(S9Vdc_timer, MsToOSTicks(2800));
    S9Vdc_timeouted = 0;
  }
}

TVoltageObj* S9VdcNext(TVoltageObj* Self)
{
  TVoltageLevel piState;
  TVoltageObj* next;

	if (IS_PANIC) 
		return &SPanic;	  
  piState = GetVoltage();
  switch (piState)
  {
    case pi12VDC:
      S6VdcPluseCompleted = 0;
      next = &S12Vdc;
      break;
    case pi9VDC:
      if (S6VdcPluseCompleted)
      {
        S6VdcPluseCompleted = 0;
        PluseWidth = Pluse6VdcEnd - Pluse6VdcBegin;  
        if ((MsToOSTicks(200)<=PluseWidth)&&(PluseWidth<=MsToOSTicks(3000))) //EV requests digital IF
        {
          next = &SDigitalIf;
          goto _exit;
        }
      }
      next = S9Vdc_timeouted? &S9Vac: &S9Vdc;
      break;
    case pi6VDC:
      Pluse6VdcBegin = osKernelGetTickCount();
      S6VdcPluseCompleted = 0;
      next = &S6Vdc;
      break;
    default:
      S6VdcPluseCompleted = 0;
      next = &SError;
      break;
  }
  _exit:  
  next->Prev = Self;
  return next;
}

void S6VdcAction(TVoltageObj* Self)
{
  EnergyOutput(OFF);
  PWM_SetDutyCycle(1); //PWM output 12VDC
  PWM_ExtTriggerDisable();  
  osDelay(VOLTAGE_CHANGE_DLY); //wait for voltage stable      
  if (Self->Prev != Self) //transit from 12VDC to 6VDC directly indicating EV without S2
  {    
    osTimerStart(S6Vdc_timer, MsToOSTicks(2800));
    S6Vdc_timeouted = 0;
  }  
}

TVoltageObj* S6VdcNext(TVoltageObj* Self)
{
  TVoltageLevel piState;
  TVoltageObj* next;
  
  if (IS_PANIC) 
		return &SPanic;	
  piState = GetVoltage();
  switch (piState)
  {
    case pi12VDC:
      next = &S12Vdc;
      break;
    case pi9VDC:
      Pluse6VdcEnd = osKernelGetTickCount();
      S6VdcPluseCompleted = 1;
      next = &S9Vdc;
      break;
    case pi6VDC:
      if (S6Vdc_timeouted && GetCapacityOffer(REQUEST_MAX_CAPACITY))
      {
        next = &S6Vac;
        Trans_SetState(trAuthen, 0);
      }
      else
        next = Self;
      break;
    default:
      next = &SError;
      break;
  }
  next->Prev = Self;
  return next;
}

static uint8_t ChargingStopped;

static int IsOverCurrent(uint16_t current, uint16_t offer)
{
  if (current<=20.0)
  {
    if (((offer==0)&&(current>=1))||(current>offer+2))
      return 1;
  }
  else if (current>offer*1.1)
    return 1;
  return 0;
}

void S6VacAction(TVoltageObj* Self)
{
  static float phases[3], total;
  static uint8_t overCurrent;
  static uint8_t ChargingStarted;
  
  if (Self->Prev == &S6Vdc) //transit from 12VDC to 6VDC directly indicating EV has no S2 switch
  {    
    osTimerStart(S6Vdc_timer, MsToOSTicks(2800));
    S6Vdc_timeouted = 0;
  }
  if (Self->Prev != Self) //transit from other state
  {
    ResetChargingCounter();
    ChargingStarted = ChargingStopped = 0;
    osTimerStop(OverCurrent_timer);
    OverCurrent_timeouted = 0;
    CapacityOffered = ChargeCurrentMax;
  }
  CapacityOffered = GetCapacityOffer(CapacityOffered);
  PWM_SetDutyCycle(GetDutyCycle(CapacityOffered)); // PWM output Ac
  osDelay(VOLTAGE_CHANGE_DLY); //wait for voltage stablize   

  total = GetChargingCurrents(phases);
  if (TrState==trCharging)
  {
    if (S6Vdc_timeouted || !EVWithoutS2)
    {
      EnergyOutput(ON);      
      if (IsDrawingCurrent(total))
      {
        ChargingStarted = 1;
        ChargingStopped = 0;
      }
      else if (ChargingStarted && (CapacityOffered>0))
        ChargingStopped = 1;
    }
  }
  //test for over current
  overCurrent = 0;  
  for (int i=0; i<3; i++)
  {
    overCurrent = IsOverCurrent(phases[i], SINGLE_PH_CURRENT_MAX);
    if (overCurrent)
    {
      if (CapacityOffered > SINGLE_PH_CURRENT_MAX)
        CapacityOffered = SINGLE_PH_CURRENT_MAX;
    }
  }
  if (!overCurrent)
    overCurrent = IsOverCurrent(total, CapacityOffered);
  
  if (!overCurrent)
  {
    if (osTimerIsRunning(OverCurrent_timer))
      osTimerStop(OverCurrent_timer);
    OverCurrent_timeouted = 0;
  }
  else if (!osTimerIsRunning(OverCurrent_timer) && !OverCurrent_timeouted)
  {
    osTimerStart(OverCurrent_timer, MsToOSTicks(5000));        
    OverCurrent_timeouted = 0;
  }
}

TVoltageObj* S6VacNext(TVoltageObj* Self)
{
  TVoltageLevel piState;
  TVoltageObj* next;
  
  if (IS_PANIC) 
		return &SPanic;	
  if (!IS_CABLE_OK)
  {
    next = IS_CONTACTOR_ON? &SError: &SStop;
    goto _exit;
  }  
  //over current or fully charged(EV without S2)
  if (OverCurrent_timeouted||(ChargingStopped && EVWithoutS2)||((TrState!=trAuthen)&&(TrState!=trCharging)))
    return &SStop;
    
  piState = GetVoltage();
  switch (piState)
  {
    case pi12VDC:
    case pi12VAC: //unplugged
      next = (IS_CONTACTOR_ON && !EVWithoutS2)? &SError: &SStop;
      break;
    case pi9VDC:
    case pi9VAC: //fully charged, car initiate stop
      next = &SStop;
      break;
    case pi6VDC: //no capacity or over temperature
      next = Self;
      break;
    case pi6VAC: //waiting for authentication or charging
      Trans_SetState(trAuthen, 0);
      next = Self;
      break;
    default:
      next = &SError;
      break;
  }
  _exit:
  next->Prev = Self;  
	
  return next;
}

void S9VacAction(TVoltageObj* Self)
{
  EnergyOutput(OFF);
  EVWithoutS2 = 0;
  PWM_SetDutyCycle(GetDutyCycle(GetCapacityOffer(REQUEST_MAX_CAPACITY))); //PWM output AC
  osDelay(VOLTAGE_CHANGE_DLY); //wait for voltage stable   
}

TVoltageObj* S9VacNext(TVoltageObj* Self)
{
  TVoltageLevel piState;
  TVoltageObj* next;

  if (IS_PANIC) 
		return &SPanic;	
  if (!IS_CABLE_OK)
  {
    next = &S9Vdc;
    goto _exit;
  }
  piState = GetVoltage();
  switch (piState)
  {
    case pi12VDC:
    case pi12VAC: //unplugged
      next = &S12Vac;
      break;
    case pi9VDC: //no capacity
    case pi9VAC: //got capacity
      next = Self;
      break;
    case pi6VDC:
    case pi6VAC:
      next = &S6Vac;    
      Trans_SetState(trAuthen, 0);
      break;
    default:
      next = &SError;
      break;
  }
  _exit:
  next->Prev = Self;  
  return next;
}

void S12VacAction(TVoltageObj* Self)
{
  EnergyOutput(OFF);
  PWM_SetDutyCycle(1); //output 12VDC
  PWM_ExtTriggerDisable();
  osDelay(VOLTAGE_CHANGE_DLY); //wait voltage for stable   
}

TVoltageObj* S12VacNext(TVoltageObj* Self)
{
  TVoltageLevel piState;
  TVoltageObj* next;
  
	if (IS_PANIC) 
		return &SPanic;	
  piState = GetVoltage();
  switch (piState)
  {
    case pi12VDC:
      next = &S12Vdc;
      break;
    case pi9VDC:
      next = &S9Vdc;
      break;
    case pi6VDC:
      next = &S6Vdc;
      break;
    default:
      next = &SError;
      break;
  }
  next->Prev = Self;  
  return next;
}

void SStopAction(TVoltageObj* Self)
{
  PWM_SetDutyCycle(1); //output 12VDC (charger not available)
  PWM_ExtTriggerDisable();
  osDelay(VOLTAGE_CHANGE_DLY); //wait voltage to stablize   
  if (Self->Prev != Self)
  {
    osTimerStart(SStop_timer, MsToOSTicks(3010));
    SStop_timeouted = 0;
  }  
}

TVoltageObj* SStopNext(TVoltageObj* Self)
{
  TVoltageLevel piState;
  TVoltageObj* next;
  
  if (IS_PANIC) 
		return &SPanic;		
  piState = GetVoltage();
  switch (piState)
  {
    case pi12VDC: //unplugged
      EnergyOutput(OFF); 
      Trans_SetState(trBilling, 0);
      next = &S12Vdc;
      break;
    case pi9VDC: //plugged, EVWithoutS2==0
      EnergyOutput(OFF);
      Trans_SetState(trParking, 0);
      next = &SStop;    
      break;
    case pi6VDC: //plugged, EVWithoutS2==1, plugged
      if (SStop_timeouted)
      {
        EnergyOutput(OFF);
        Trans_SetState(trParking, 0);
      }
      next = &SStop;
      break;
    default:
      EnergyOutput(OFF);
      Trans_SetState(trParking, 0);
      next = &SError;
      break;
  }
  next->Prev = Self;  
  return next;
}

void SErrorAction(TVoltageObj* Self)
{
  EnergyOutput(OFF);
  PWM_SetDutyCycle(0); //output -12VDC (charger fault)
  PWM_ExtTriggerDisable();
  osDelay(VOLTAGE_CHANGE_DLY); //wait for voltage stable
  SolenoidOpen();
  if ((TrState<=trAuthen)||(TrState==trPaid))
    Trans_SetState(trIdle, 0);
  else if ((TrState==trCharging)||(TrState==trParking))
    Trans_SetState(trBilling, 0);
}

TVoltageObj* SErrorNext(TVoltageObj* Self)
{
  return &SError;
}

void SPanicAction(TVoltageObj* Self)
{
  EnergyOutput(OFF);
  PWM_SetDutyCycle(0); //output -12VDC (charger fault)
  PWM_ExtTriggerDisable();
  osDelay(VOLTAGE_CHANGE_DLY); //wait voltage for stable   
  SolenoidOpen();
  if ((TrState<=trAuthen)||(TrState==trPaid))
    Trans_SetState(trIdle, 0);
  else if ((TrState==trCharging)||(TrState==trParking))
    Trans_SetState(trBilling, 0);
}

TVoltageObj* SPanicNext(TVoltageObj* Self)
{
  return IS_PANIC? &SPanic: &SStop;
}

void SDigitalIfAction(TVoltageObj* Self)
{
  EnergyOutput(OFF);
  PWM_SetDutyCycle(1); //output 12VDC (charger not available)
  PWM_ExtTriggerDisable();
  SolenoidOpen();
} 

TVoltageObj* SDigitalIfNext(TVoltageObj* Self)
{
  TVoltageLevel piState;
  TVoltageObj* next;
  
  piState = GetVoltage();
  switch (piState)
  {
    case pi12VDC:
      next = &S12Vdc;
      break;
    case pi9VDC:
    case pi6VDC:
      next = &SDigitalIf;
      break;
    default:
      next = &SError;
      break;
  }
  next->Prev = Self;  
  return next;
}

static void Timer_Callback(void *arg)
{
  *((uint32_t*)arg) = 1;
}

void InitStateMachine(void)
{
  PWM_timer = osTimerNew(&Timer_Callback, osTimerOnce, &PWM_timeouted, NULL);  
  while (PWM_timer == NULL);

  S9Vdc_timer = osTimerNew(&Timer_Callback, osTimerOnce, &S9Vdc_timeouted, NULL);  
  while (S9Vdc_timer == NULL);

  S6Vdc_timer = osTimerNew(&Timer_Callback, osTimerOnce, &S6Vdc_timeouted, NULL);  
  while (S6Vdc_timer == NULL);
  
  SStop_timer = osTimerNew(&Timer_Callback, osTimerOnce, &SStop_timeouted, NULL);  
  while (SStop_timer == NULL);
  
  Solenoid_timer = osTimerNew(&Timer_Callback, osTimerOnce, &Solenoid_timeouted, NULL);  
  while (Solenoid_timer == NULL);
  
  OverCurrent_timer = osTimerNew(&Timer_Callback, osTimerOnce, &OverCurrent_timeouted, NULL);  
  while (OverCurrent_timer == NULL);
}

TVoltageObj* ReadCurStateObj(void)
{
  MASK_IRQ
    TVoltageObj* ret = CurState;
  UNMASK_IRQ
  return ret;
}

static void SetCurStateObj(TVoltageObj *vState)
{
  MASK_IRQ
    CurState = vState;
  UNMASK_IRQ
}

int IsChargingReady(void)
{
  TVoltageObj *vState = ReadCurStateObj();
  return !IsCoverOpended() && !IsTempOutOfRange() && ((vState==&S6Vac)||(vState==&S9Vac));
}

int IsCharging(void)
{
  return IS_CONTACTOR_ON;
}

int IsFatalError(void)
{
	return ReadCurStateObj()==&SError;
}

int IsPanic(void)
{
  return ReadCurStateObj()==&SPanic;
}

int IsDigitalNeeded(void)
{
  return ReadCurStateObj()==&SDigitalIf;
}

void PilotThread(void *arg)
{
  static TVoltageObj* vState = &S12Vdc;
  static uint32_t flags;
  static uint32_t secCount;
  
  InitStateMachine();
  while(1)
  {        
    TrState = Trans_GetState();
    vState->Action(vState);
    vState = vState->Next(vState);  
    SetCurStateObj(vState);
        
    flags = osThreadFlagsGet();
    if (flags & PILOT_UPD_VARS)
    {
      LoadCapacityVars();
      osThreadFlagsClear(PILOT_UPD_VARS);
    }  
    else if (flags & PILOT_RESET_ERROR)
    {
      if (IsFatalError())
      {
        vState = &S12Vdc;
        SetCurStateObj(vState);
      }
      osThreadFlagsClear(PILOT_RESET_ERROR);
    }
    
    if (secCount++ >= 1000/(PILOT_STATE_DLY*MsPerTick))
    {
      secCount = 0;
      if (TrState==trCharging)
      {
        if (Trans_IsCreditOut())
          Trans_SetState(trParking, 0);      
      }
      Trans_PaidStateDelayTick();
   }
      
    osDelay(PILOT_STATE_DLY); 
  }
}

