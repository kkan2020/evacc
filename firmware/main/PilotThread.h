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
 
#ifndef __STATE_H__
#define __STATE_H__

#define PILOT_UPD_VARS          0x01
#define PILOT_RESET_ERROR       0x02

typedef enum {mmDC, mmPeak, mmCrest} TMeasurement;
typedef enum {pwmError, pwm6V, pwm9V, pwm12V, pwmN12V, pwmTimeouted} TPWMLevel;
typedef enum 
{
  piError, 
  pi6VDC, pi9VDC, pi12VDC, piN12VDC, 
  pi6VAC, pi9VAC, pi12VAC, piN12VAC, 
  piTimeouted, piTimerError
} TVoltageLevel;

typedef struct TVoltageObj
{
  char* Name;
  struct TVoltageObj* Prev;
  void (*Action)(struct TVoltageObj* Self);
  struct TVoltageObj* (*Next)(struct TVoltageObj* Self);
} TVoltageObj;

extern TVoltageObj S12Vdc, S9Vdc, S6Vdc, S6Vac, S9Vac, S12Vac, SError, SStop, SPanic, SDigitalIf;
extern __IO uint16_t CapacityReserved;
extern const osThreadAttr_t PilotThread_attr;


//void LoadCapacityVars(void);
//void InitStateMachine(void);

uint8_t IsVoltageStateChanged(void);
void ResetVoltageState(void);
TVoltageObj* ReadCurStateObj(void);
int IsCharging(void);
int IsChargingReady(void);
int IsFatalError(void);
int IsPanic(void);
int IsDigitalNeeded(void);
void PilotThread(void *arg);
#ifdef EMULATION_ENABLED
void SetVoltage(TVoltageLevel state);
#endif

#endif


